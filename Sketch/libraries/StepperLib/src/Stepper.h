////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2018 Herbert Aitenbichler

  CNCLib is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  CNCLib is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  http://www.gnu.org/licenses/
*/
////////////////////////////////////////////////////////

#pragma once

#include "ConfigurationStepperLib.h"
#include "HAL.h"
#include "RingBuffer.h"
#include "Singleton.h"
#include "UtilitiesStepperLib.h"

////////////////////////////////////////////////////////
//
// assume step <==> mm
// a  ... acceleration
// d  ... distance in steps 
// v  ... velocity in steps/sec
// v0 ... velocity at start <=> a
// F  ... TimerFrequency in Hz (Arduino 1/8 CPU => 2MHZ)
// c  ... timer value (for step) in multiply of timerFrequency (NOT sec)
// v  ... (1 / F) * c
//
////////////////////////////////////////////////////////

class CStepper : public CSingleton<CStepper>
{
public:
	/////////////////////

	enum EWaitType
	{
		MovementQueueFull,
		WaitBusyCall,

		WaitTimeCritical,										// TimeCritical wait types start here
		WaitReference
	};

	enum EStepperEvent
	{
		OnStartEvent,											// Stepper, Io and Wait
		OnIdleEvent,
		OnDisableEvent,											// Disable stepper if inactive
		OnWaitEvent,
		OnErrorEvent,
		OnWarningEvent,
		OnInfoEvent,
		OnIoEvent,

		LastStepperEvent = OnIoEvent
	};

	#define LevelToProcent(a) (a*100/255)
	#define ProcentToLevel(a) (a*255/100)

	enum ELevel
	{	
		LevelMax=255,
		Level20P = ProcentToLevel(20),
		Level60P = ProcentToLevel(60),
		LevelOff = 0
	};

	enum ESpeedOverride
	{
		SpeedOverrideMax = 255,
		SpeedOverride100P = 128,
		SpeedOverrideMin = 1
	};

	enum EDumpOptions		// use bit
	{
		DumpAll			= 0xff,
		DumpPos			= 1,
		DumpState		= 2,
		DumpMovements	= 8,
		DumpDetails		= 128									// detail of each option
	};

	typedef bool(*StepperEvent)(CStepper*stepper, uintptr_t param, EnumAsByte(EStepperEvent) eventtype, uintptr_t addinfo);
	typedef bool(*TestContinueMove)(uintptr_t param);

	struct SEvent 
	{
		SEvent()																						{ _event = NULL;  _eventParam = 0; }
		StepperEvent  _event;
		uintptr_t 	  _eventParam;
		bool Call(CStepper*stepper, EnumAsByte(CStepper::EStepperEvent) eventtype, uintptr_t addinfo)	{ if (_event) return _event(stepper, _eventParam, eventtype, addinfo); return true; }
	};

	struct SIoControl
	{
		uint8_t			_tool;
		unsigned short	_level;
	};

	/////////////////////

protected:

	CStepper();

public:

	virtual void Init();

	bool IsError()												{ return _pod._error != 0; };
	error_t GetError()											{ return _pod._error; }
	void ClearError()											{ _pod._error = 0; }

	bool IsFatalError()											{ return _pod._fatalerror != 0; };
	error_t GetFatalError()										{ return _pod._fatalerror; }
	void ClearFatalError()										{ _pod._fatalerror = 0; }


	void SetMaxSpeed(axis_t axis, steprate_t vMax)				{ _pod._timerMax[axis] = SpeedToTimer(vMax); }
	void SetAcc(axis_t axis, steprate_t v0Acc)					{ _pod._timerAcc[axis] = SpeedToTimer(v0Acc); }
	void SetDec(axis_t axis, steprate_t v0Dec)					{ _pod._timerDec[axis] = SpeedToTimer(v0Dec); }
	void SetAccDec(axis_t axis, steprate_t v0Acc, steprate_t v0Dec) { SetAcc(axis, v0Acc); SetDec(axis, v0Dec); }

	void SetDefaultMaxSpeed(steprate_t vMax)					{ _pod._timerMaxDefault = SpeedToTimer(vMax); }
	void SetDefaultMaxSpeed(steprate_t vMax, steprate_t v0Acc, steprate_t v0Dec)				{ SetDefaultMaxSpeed(vMax); for (axis_t i = 0; i < NUM_AXIS; i++) { SetAcc(i, v0Acc); SetDec(i, v0Dec); } }
	void SetDefaultMaxSpeed(steprate_t vMax, axis_t axis, steprate_t v0Acc, steprate_t v0Dec)	{ SetDefaultMaxSpeed(vMax); SetAccDec(axis, v0Acc, v0Dec); }

	void SetDefaultMaxSpeed(steprate_t vMax, steprate_t v0Acc, steprate_t v0Dec, steprate_t vMaxJerk)	{ SetDefaultMaxSpeed(vMax); for (axis_t i = 0; i < NUM_AXIS; i++) { SetAcc(i, v0Acc); SetDec(i, v0Dec); SetJerkSpeed(i, vMaxJerk);} }
	
	void SetSpeedOverride(EnumAsByte(ESpeedOverride) speed)		{ _pod._speedoverride = speed; }
	EnumAsByte(ESpeedOverride) GetSpeedOverride()				{ return _pod._speedoverride; }

	static uint8_t SpeedOverrideToP(EnumAsByte(ESpeedOverride) speed)	  {	return RoundMulDivU8((uint8_t) speed, 100, SpeedOverride100P);	}
	static  EnumAsByte(ESpeedOverride) PToSpeedOverride(uint8_t speedP) { return (EnumAsByte(ESpeedOverride)) RoundMulDivU8(speedP, SpeedOverride100P, 100); }

	void SetUsual(steprate_t vMax);

	void SetJerkSpeed(axis_t axis, steprate_t vMaxJerk)			{ _pod._maxJerkSpeed[axis] = vMaxJerk; }
	
	void SetWaitFinishMove(bool wait)                           { _pod._waitFinishMove = wait; };
	bool IsWaitFinishMove() const								{ return _pod._waitFinishMove; }

	void SetCheckForReference(bool check)                       { _pod._checkReference = check; };
	bool IsCheckForReference()  const							{ return _pod._checkReference; }

	void SetBacklash(steprate_t speed)			                { _pod._timerbacklash = SpeedToTimer(speed); };
	bool IsSetBacklash()  const									{ return ((timer_t)-1) != _pod._timerbacklash; }
	steprate_t GetBacklash()									{ return TimerToSpeed(_pod._timerbacklash); };

	bool IsBusy()  const										{ return _pod._timerRunning; };
	void WaitBusy();

	bool CanQueueMovement()	 const								{ return !_movements._queue.IsFull(); }
	uint8_t QueuedMovements()	 const							{ return _movements._queue.Count(); }

	uint8_t GetEnableTimeout(axis_t axis) const					{ return _pod._timeOutEnable[axis]; }
	void SetEnableTimeout(axis_t axis, uint8_t sec) 			{ _pod._timeOutEnable[axis] = sec; }

	void SetDirection(axisArray_t direction)					{ _pod._invertdirection = direction; }

#ifdef USESLIP
	void SetSlip(int dist[NUM_AXIS]);
#endif

	void SetLimitMax(axis_t axis, udist_t limit)				{ _pod._limitMax[axis] = limit; };
#ifndef REDUCED_SIZE
	void SetLimitMin(axis_t axis, udist_t limit)				{ _pod._limitMin[axis] = limit; };
#endif

	void SetBacklash(axis_t axis, mdist_t dist)					{ _pod._backlash[axis] = dist; }

	void StopMove(steprate_t v0Dec=0);							// Stop all pendinge/current moves, WITH dec ramp, clear buffer
	void AbortMove();											// Abort all pendinge/current moves, NO dec ramp, clear buffer

	void PauseMove();											// Finish current move but do not continue with next (WITH dec ramp)
	void ContinueMove();										// continue after pause
	bool IsPauseMove()											{ return _pod._pause;  }

	void EmergencyStop()										{ _pod._emergencyStop = true; AbortMove(); }
	bool IsEmergencyStop()										{ return _pod._emergencyStop; }
	void EmergencyStopResurrect();

	bool IsWaitConditional()									{ return _pod._isWaitConditional; }
	void SetWaitConditional(bool conditionalwait)				{ _pod._isWaitConditional = conditionalwait; }

	void SetReferenceHitValue(uint8_t referneceid, uint8_t valueHit)	{ _pod._referenceHitValue[referneceid] = valueHit; }
	bool IsUseReference(uint8_t referneceid)					{ return _pod._referenceHitValue[referneceid] != 255; }
	bool IsUseReference(axis_t axis, bool toMin)				{ return IsUseReference(ToReferenceId(axis, toMin)); }

	debugvirtula bool MoveReference(axis_t axis, uint8_t referenceid, bool toMin, steprate_t vMax, sdist_t maxdist = 0, sdist_t distToRef = 0, sdist_t distIfRefIsOn = 0);
	void SetPosition(axis_t axis, udist_t pos);

	//////////////////////////////

	void MoveAbs(const udist_t d[NUM_AXIS], steprate_t vMax = 0);
	void MoveRel(const sdist_t d[NUM_AXIS], steprate_t vMax = 0);

	void MoveAbs(axis_t axis, udist_t d, steprate_t vMax = 0);
	void MoveRel(axis_t axis, sdist_t d, steprate_t vMax = 0);

	void MoveAbsEx(steprate_t vMax, unsigned short axis, udist_t d, ...);	// repeat axis and d until axis not in 0 .. NUM_AXIS-1
	void MoveRelEx(steprate_t vMax, unsigned short axis, sdist_t d, ...);	// repeat axis and d until axis not in 0 .. NUM_AXIS-1
	void Wait(unsigned int sec100);							// unconditional wait
	void WaitConditional(unsigned int sec100);				// conditional wait 
	void IoControl(uint8_t tool, unsigned short level);

	bool MoveUntil(TestContinueMove testcontinue, uintptr_t param);

	//////////////////////////////

	const udist_t* GetPositions() const							{ return _pod._calculatedpos; }
	void GetPositions(udist_t pos[NUM_AXIS]) const				{ memcpy(pos, _pod._calculatedpos, sizeof(_pod._calculatedpos)); }
	udist_t GetPosition(axis_t axis) const						{ return _pod._calculatedpos[axis]; }

	void GetCurrentPositions(udist_t pos[NUM_AXIS]) const		{ CCriticalRegion crit; memcpy(pos, _pod._current, sizeof(_pod._current)); }
	udist_t GetCurrentPosition(axis_t axis) const				{ CCriticalRegion crit; return (*((volatile udist_t*)&_pod._current[axis])); }

	udist_t GetLimitMax(axis_t axis) const						{ return _pod._limitMax[axis]; }
#ifdef REDUCED_SIZE
	udist_t GetLimitMin(axis_t ) const							{ return 0; }
#else
	udist_t GetLimitMin(axis_t axis) const						{ return _pod._limitMin[axis]; }
#endif

	mdist_t GetBacklash(axis_t axis) const						{ return _pod._backlash[axis]; }
	axisArray_t GetLastDirection() const						{ return _pod._lastdirection; }		// check for backlash

	steprate_t GetDefaultVmax() const							{ return TimerToSpeed(_pod._timerMaxDefault); }
	steprate_t GetMaxSpeed(axis_t axis) const					{ return min(GetDefaultVmax(),TimerToSpeed(_pod._timerMax[axis])); }
	steprate_t GetAcc(axis_t axis) const						{ return TimerToSpeed(_pod._timerAcc[axis]); }
	steprate_t GetDec(axis_t axis) const						{ return TimerToSpeed(_pod._timerDec[axis]); }
	steprate_t GetJerkSpeed(axis_t axis) const					{ return _pod._maxJerkSpeed[axis]; }

#ifndef REDUCED_SIZE
	unsigned long GetTotalSteps() const							{ return _pod._totalSteps; }
	unsigned int GetTimerISRBuys() const						{ return _pod._timerISRBusy; }
#endif
	unsigned long IdleTime() const								{ return _pod._timerStartOrOnIdle; }

	void AddEvent(StepperEvent event, uintptr_t eventparam, SEvent&old );

	static uint8_t ToReferenceId(axis_t axis, bool minRef) { return axis * 2 + (minRef ? 0 : 1); }

	virtual bool  IsAnyReference() = 0;
	virtual uint8_t  GetReferenceValue(uint8_t referenceid) = 0;
	bool IsReferenceTest(uint8_t referenceid) { return GetReferenceValue(referenceid) == _pod._referenceHitValue[referenceid]; }

	void SetEnableAll(uint8_t level);				// level 0-255
	virtual void SetEnable(axis_t axis, uint8_t level, bool force) = 0;
	virtual uint8_t GetEnable(axis_t axis) = 0;

	static uint8_t ConvertLevel(bool enable)				{ return enable ? (uint8_t)(LevelMax) : (uint8_t)(LevelOff); }

	void Dump(uint8_t options);							// options ==> EDumpOptions with bits

	////////////////////////////////////////////////////////

private:

	void SetTimeoutAndEnable(axis_t i, uint8_t timeout, uint8_t level, bool force);

	void QueueMove(const mdist_t dist[NUM_AXIS], const bool directionUp[NUM_AXIS], timer_t timerMax, uint8_t stepmult);
	void QueueWait(const mdist_t dist, timer_t timerMax, bool checkCondition);

	void EnqueuAndStartTimer(bool waitfinish);
	void WaitUntilCanQueue();
	bool StartMovement();

	long CalcNextPos(udist_t current, udist_t dist, bool directionUp)
	{
		if (directionUp) return (sdist_t)current + (sdist_t)dist;
		return (sdist_t)current - (sdist_t)dist;
	}

	inline void StepOut();
	inline void StartBackground();
	inline void FillStepBuffer();
	void Background();

	void GoIdle();
	void ContinueIdle();

	void CallEvent(EnumAsByte(EStepperEvent) eventtype, uintptr_t addinfo=0)	{ _event.Call(this, eventtype, addinfo); }

	void SubTotalSteps();

protected:

	bool MoveUntil(uint8_t referenceId, bool referencevalue, unsigned short stabletime);

	void QueueAndSplitStep(const udist_t dist[NUM_AXIS], const bool directionUp[NUM_AXIS], steprate_t vMax);

	debugvirtula void StepRequest(bool isr);
	debugvirtula void OptimizeMovementQueue(bool force);

	////////////////////////////////////////////////////////

	timer_t GetTimer(mdist_t steps, timer_t timerstart);										// calc "speed" after steps with constant a (from v0 = 0)
	timer_t GetTimerAccelerating(mdist_t steps, timer_t timerv0, timer_t timerstart);			// calc "speed" after steps accelerating with constant a 
	timer_t GetTimerDecelerating(mdist_t steps, timer_t timer, timer_t timerstart);				// calc "speed" after steps decelerating with constant a 

	static mdist_t GetAccSteps(timer_t timer, timer_t timerstart);										// from v=0
	static mdist_t GetDecSteps(timer_t timer, timer_t timerstop)								{ return GetAccSteps(timer, timerstop); } // to v=0

	static mdist_t GetAccSteps(timer_t timer1, timer_t timer2, timer_t timerstart);				// from v1 to v2 (v1<v2)
	static mdist_t GetDecSteps(timer_t timer1, timer_t timer2, timer_t timerstop)				{ return GetAccSteps(timer2, timer1, timerstop); }

	static mdist_t GetSteps(timer_t timer1, timer_t timer2, timer_t timerstart, timer_t timerstop);		// from v1 to v2 (v1<v2 uses acc, dec otherwise)

	unsigned long GetAccelerationFromTimer(mdist_t timerV0);
	unsigned long GetAccelerationFromSpeed(steprate_t speedV0)									{ return GetAccelerationFromSpeed(SpeedToTimer(speedV0)); }

	timer_t SpeedToTimer(steprate_t speed) const;
	steprate_t TimerToSpeed(timer_t timer) const;

	static uint8_t GetStepMultiplier(timer_t timermax);

protected:

	//////////////////////////////////////////
	// often accessed members first => is faster
	// even size of struct and 2byte alignement

	struct POD														// POD .. Plane Old Data Type => no Constructor => init with default value = 0
	{
		volatile bool	_timerRunning;
		bool			_checkReference;							// check for "IsReference" in ISR (while normal move)

		bool			_emergencyStop;
		bool			_isWaitConditional;							// wait on "Wait"

		bool			_waitFinishMove;
		bool			_limitCheck;

		timer_t			_timerbacklash;								// -1 or 0 for temporary enable/disable backlash without setting _backlash to 0

#ifndef REDUCED_SIZE
		unsigned long	_totalSteps;								// total steps since start
		unsigned int	_timerISRBusy;								// ISR while in ISR
#endif

		timer_t			_timerMaxDefault;							// timervalue of vMax (if vMax = 0)

		udist_t			_current[NUM_AXIS];							// update in ISR
		udist_t			_calculatedpos[NUM_AXIS];					// calculated in advanced (use movement queue)

		uint8_t			_referenceHitValue[NUM_REFERENCE];			// each axis min and max - used in ISR LOW,HIGH, 255(not used)

		steprate_t		_maxJerkSpeed[NUM_AXIS];					// immediate change of speed without ramp (in junction)

		timer_t			_timerMax[NUM_AXIS];						// maximum speed of axis
		timer_t			_timerAcc[NUM_AXIS];						// acc timer start
		timer_t			_timerDec[NUM_AXIS];						// dec timer start

#ifndef REDUCED_SIZE
		udist_t			_limitMin[NUM_AXIS];
#endif
		udist_t			_limitMax[NUM_AXIS];

		mdist_t			_backlash[NUM_AXIS];						// backlash of each axis (signed mdist_t/2)

		unsigned long	_timerStartOrOnIdle;						// timervalue if library start move or goes to Idle
		unsigned long	_timerLastCheckEnable;						// timervalue

		uint8_t	_idleLevel;											// level if idle (0..100)
		volatile EnumAsByte(ESpeedOverride)	_speedoverride;			// Speed override, 128 => 100% (change in irq possible)

		axisArray_t		_lastdirection;								// for backlash
		axisArray_t		_invertdirection;							// invert direction

		error_t			_error;
		error_t			_fatalerror;

		uint8_t			_timeOutEnable[NUM_AXIS];					// enabletimeout in sec if no step (0.. disable, always enabled)
		uint8_t			_timeEnable[NUM_AXIS];						// 0: active, do not turn off, else time to turn off

#ifdef USESLIP
		unsigned int _slipSum[NUM_AXIS];
		int _slip[NUM_AXIS];
#endif

		bool		_pause;											// PauseMove is called
		axisArray_t	_lastDirectionUp;								// last paramter value of Steo()

	} _pod;

	SEvent		_event ALIGN_WORD;									// no POS => Constructor
	axis_t		_num_axis;											// actual axis (3 e.g. SMC800)

	struct SMovementState;

	/////////////////////////////////////////////////////////////////////
	// internal ringbuffer for movement optimization

	struct SMovement
	{
	protected:
		friend class CStepper;

		enum EMovementState
		{
			StateReadyMove	= 1,								// ready for travel (not executing)
			StateReadyWait	= 2,								// ready for none "travel" move (wait move) (not executing)
			StateReadyIo	= 3,								// ready for none "travel" move (set io) (not executing)

			StateUpAcc		= 11,								// in start phase accelerate
			StateUpDec		= 12,								// in start phase decelerate to vmax
			StateRun		= 13,								// running (no acc and dec)
			StateDownDec	= 14,								// in stop phase decelerate
			StateDownAcc	= 15,								// in stop phase accelerate

			StateWait		= 21,								// executing wait (do no step)

			StateDone		= 0,								// finished
		};

		mdist_t		_steps;										// total movement steps (=distance)

		EnumAsByte(EMovementState) _state;						// emums are 16 bit in gcc => force byte
		bool		_backlash;									// move is backlash

		DirCount_t	_dirCount;
		DirCount_t	_lastStepDirCount;

		mdist_t		_distance_[NUM_AXIS];						// distance adjusted with stepmultiplier => use GetDistance(axis)

		struct SRamp											// only modify in CCRiticalRegion
		{
			timer_t _timerStart;								// start ramp with speed (tinmerValue)
			timer_t _timerRun;
			timer_t _timerStop;									// stop  ramp with speed (timerValue)

			mdist_t _upSteps;									// steps needed for accelerating from v0
			mdist_t _downSteps;									// steps needed for decelerating to v0
			mdist_t _downStartAt;								// index of step to start with deceleration

			mdist_t _nUpOffset;									// offset of n rampe calculation(acc) 
			mdist_t _nDownOffset;								// offset of n rampe calculation(dec)

			void RampUp(SMovement* pMovement, timer_t timerRun, timer_t timerJunction);
			void RampDown(SMovement* pMovement, timer_t timerJunction);
			void RampRun(SMovement* pMovement);
		};

		union
		{
			struct SMove
			{
				timer_t _timerMax;										// timer for max requested speed
				timer_t _timerRun;										// copy of _ramp. => modify during rampcalc

				timer_t _timerEndPossible;								// timer possible at end of last movement
				timer_t _timerJunctionToPrev;							// used to calculate junction speed, stored in "next" step
				timer_t _timerMaxJunction;								// max possible junction speed, stored in "next" step

				struct SRamp _ramp;										// only modify in CCRiticalRegion

				timer_t _timerAcc;										// timer for calc of acceleration while "up" state - depend on axis
				timer_t _timerDec;										// timer for calc of decelerating while "down" state - depend on axis
			} _move;

			struct SWait
			{
				timer_t _timer;
				bool _checkWaitConditional;								// wait only if Stepper.SetConditionalWait is set
			} _wait;

			struct SIoControl _io;

		} _pod;

		stepperstatic CStepper* _pStepper;						// give access to stepper (not static if multiinstance)  

		timer_t GetUpTimerAcc()									{ return _pod._move._timerAcc; }
		timer_t GetUpTimerDec()									{ return _pod._move._timerDec; }

		timer_t GetDownTimerAcc()								{ return _pod._move._timerAcc; }
		timer_t GetDownTimerDec()								{ return _pod._move._timerDec; }

		timer_t GetUpTimer(bool acc)							{ return acc ? GetUpTimerAcc() : GetUpTimerDec(); }
		timer_t GetDownTimer(bool acc)							{ return acc ? GetDownTimerAcc() : GetDownTimerDec(); }

		mdist_t GetDistance(axis_t axis);
		uint8_t GetStepMultiplier(axis_t axis)					{ return (_dirCount >> (axis * 4)) % 8; }
		bool GetDirectionUp(axis_t axis)						{ return ((_dirCount >> (axis * 4)) & 8) != 0; }
		uint8_t GetMaxStepMultiplier();

		bool Ramp(SMovement*mvNext);

		void CalcMaxJunktionSpeed(SMovement*mvNext);

		bool AdjustJunktionSpeedT2H(SMovement*mvPrev, SMovement*mvNext);
		void AdjustJunktionSpeedH2T(SMovement*mvPrev, SMovement*mvNext);

		bool CalcNextSteps(bool continues);

	private:

		bool IsEndWait() const;									// immediately end wait 

	public:

		mdist_t GetSteps()										{ return _steps; }

		bool IsActiveIo() const									{ return _state == StateReadyIo; }							// Ready from Io
		bool IsActiveWait() const								{ return _state == StateReadyWait || _state == StateWait; }	// Ready from wait or waiting
		bool IsActiveMove() const								{ return IsReadyForMove() || IsProcessingMove(); }			// Ready from move or moving
		bool IsReadyForMove() const								{ return _state == StateReadyMove; }						// Ready for move but not started
		bool IsProcessingMove() const							{ return _state >= StateUpAcc && _state <= StateDownAcc; }	// Move is currently processed (in acc,run or dec)
		bool IsUpMove() const									{ return IsProcessingMove() && _state < StateRun; }			// Move in ramp acc state
		bool IsRunOrUpMove() const								{ return IsProcessingMove() && _state <= StateRun; }		// Move in ramp run state
		bool IsRunMove() const									{ return _state == StateRun; }								// Move in ramp run state
		bool IsRunOrDownMove() const							{ return IsProcessingMove() && _state >= StateRun; }		// Move in ramp run state
		bool IsDownMove() const									{ return IsProcessingMove() && _state > StateRun; }			// Move in ramp dec state
		bool IsFinished() const									{ return _state == StateDone; }								// Move finished 

		bool IsSkipForOptimizing() const						{ return IsActiveIo();  }									// skip the entry when optimizing queue

		void InitMove(CStepper*pStepper, SMovement* mvPrev, mdist_t steps, const mdist_t dist[NUM_AXIS], const bool directionUp[NUM_AXIS], timer_t timerMax);
		void InitWait(CStepper*pStepper, mdist_t steps, timer_t timer, bool checkWaitConditional);
		void InitIoControl(CStepper*pStepper, uint8_t tool, unsigned short level);

		void InitStop(SMovement* mvPrev, timer_t timer, timer_t dectimer);

		void SetBacklash()										{ _backlash = true; }

		void Dump(uint8_t queueidx, uint8_t options);

#ifdef _MSC_VER
		char _mvMSCInfo[MOVEMENTINFOSIZE];
#endif

	};

	struct SMovements
	{
		timer_t _timerStartPossible;							// timer for fastest possible start (break at the end)
		CRingBufferQueue<SMovement, MOVEMENTBUFFERSIZE>	_queue;
	};

	stepperstatic struct SMovements _movements;

	/////////////////////////////////////////////////////////////////////
	// internal state of move (used in ISR)

	struct SMovementState
	{
	protected:
		friend class CStepper;

		// state for calculating steps (moving)
		// static for performance on arduino => only one instance allowed

		mdist_t _n;				// step within movement (1-_steps)
		timer_t _timer;			// current timer
		timer_t _rest;			// rest of rampcalculation

		uint8_t _count;	// increment of _n
		char _dummyalign;

#ifndef REDUCED_SIZE
		unsigned long _sumTimer;	// for debug
#endif

		mdist_t _add[NUM_AXIS];

		void Init(SMovement* pMovement);

		bool CalcTimerAcc(timer_t maxtimer, mdist_t n, uint8_t cnt);
		bool CalcTimerDec(timer_t mintimer, mdist_t n, uint8_t cnt);

	public:

		void Dump(uint8_t options);
	};

	stepperstatic SMovementState _movementstate;

	/////////////////////////////////////////////////////////////////////
	// internal ringbuffer for steps (after calculating acc and dec)

	struct SStepBuffer
	{
	public:
		DirCount_t		DirStepCount;								// direction and count
		timer_t			Timer;
#ifdef _MSC_VER
		mdist_t			_distance[NUM_AXIS];						// to calculate relative speed
		mdist_t			_steps;
		SMovement::EMovementState  _state;
		mdist_t			_n;
		uint8_t	_count;
		char _spMSCInfo[MOVEMENTINFOSIZE];
#endif
		void Init(DirCount_t dirCount)
		{
			Timer		 = 0;
			DirStepCount = dirCount;
		};
	};

	stepperstatic CRingBufferQueue<SStepBuffer, STEPBUFFERSIZE>	_steps;

public:

#ifdef _MSC_VER
	const char* MSCInfo;
#endif

	//////////////////////////////////////////

private:

	SMovement* GetNextMovement(uint8_t idx);
	SMovement* GetPrevMovement(uint8_t idx);


protected:

	debugvirtula void OnIdle(unsigned long idletime);				// called in ISR
	debugvirtula void OnWait(EnumAsByte(EWaitType) wait);			// wait for finish move or movementqueue full
	debugvirtula void OnStart();									// startup of movement

	debugvirtula void OnError(error_t error);
	debugvirtula void OnWarning(error_t warning);
	debugvirtula void OnInfo(error_t info);

	void FatalError(error_t error)										{ _pod._fatalerror = error; }
	void Error(error_t error)											{ _pod._error = error; OnError(error); }
	void Info(error_t  info)											{ OnInfo(info); }
	void Warning(error_t warning)										{ OnWarning(warning); }

	void Error() NEVER_INLINE_AVR										{ Error(MESSAGE_UNKNOWNERROR); }
	void FatalError()													{ FatalError(MESSAGE_UNKNOWNERROR); }

protected:

	bool  MoveAwayFromReference(axis_t axis, uint8_t referenceid, sdist_t dist, steprate_t vMax);
	virtual void MoveAwayFromReference(axis_t axis, sdist_t dist, steprate_t vMax)							{ MoveRel(axis, dist, vMax); };

#ifdef _MSC_VER
	virtual void  StepBegin(const SStepBuffer* /* step */) {  };
	virtual void  StepEnd() {};
#endif

	virtual void Step(const uint8_t steps[NUM_AXIS], axisArray_t directionUp, bool isSameDirection) = 0;

	bool IsSameDirectionUp(axisArray_t directionUp)						{ return directionUp == _pod._lastDirectionUp;  }

private:

	void InitMemVar();

	////////////////////////////////////////////////////////
	// HAL

protected:

	debugvirtula void InitTimer()								{ CHAL::InitTimer1OneShot(HandleInterrupt); }
	debugvirtula void RemoveTimer()								{ CHAL::RemoveTimer1(); }

	debugvirtula void StartTimer(timer_t timerB);
	debugvirtula void SetIdleTimer();

	static void HandleInterrupt()								{ GetInstance()->StepRequest(true); }
	static void HandleBackground()								{ GetInstance()->Background(); }


	////////////////////////////////////////////////////////
	// timer supportes pin for step / dir (A4998)
protected:

	static uint8_t _mysteps[NUM_AXIS];
	static volatile uint8_t _setState;
	static uint8_t _myCnt;

	enum ESetPinState
	{
		NextIsDone=0,
		NextIsSetPin,
		NextIsClearPin,
		NextIsClearDonePin,
	};

	static void InitStepDirTimer(const uint8_t steps[NUM_AXIS])
	{
		memcpy(_mysteps, steps, sizeof(_mysteps));
		_myCnt = 0;
		_setState = NextIsSetPin;
	}
};
