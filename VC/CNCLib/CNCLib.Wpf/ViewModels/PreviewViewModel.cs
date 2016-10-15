﻿////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2016 Herbert Aitenbichler

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

using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using CNCLib.GCode;
using CNCLib.GCode.Commands;
using CNCLib.Logic.Contracts.DTO;
using Framework.Wpf.Helpers;
using Framework.Wpf.ViewModels;

namespace CNCLib.Wpf.ViewModels
{
	public class PreviewViewModel : BaseViewModel
	{
		#region crt

		public PreviewViewModel()
		{
		}

		#endregion

		#region Properties

		public Framework.Arduino.ArduinoSerialCommunication Com
		{
			get { return Framework.Tools.Pattern.Singleton<Framework.Arduino.ArduinoSerialCommunication>.Instance; }
		}

		private CommandList _commands = new CommandList();

		public CommandList Commands
		{
			get { return _commands; }
			set { SetProperty(() => _commands == value, () => _commands = value); }
		}

		private decimal _offsetX = 0;

		public decimal OffsetX
		{
			get { return _offsetX; }
			set { SetProperty(() => _offsetX == value, () => _offsetX = value); }
		}

		private decimal _offsetY = 0;
		public decimal OffsetY
		{
			get { return _offsetY; }
			set { SetProperty(() => _offsetY == value, () => _offsetY = value); }
		}

		private double _zoom = 1;
		public double Zoom
		{
			get { return _zoom; }
			set { SetProperty(() => _zoom == value, () => _zoom = value); }
		}

		private decimal _laserSize = 0.25m;
		public decimal LaserSize
		{
			get { return _laserSize; }
			set { SetProperty(() => _laserSize == value, () => _laserSize = value); }
		}

		private Color _machineColor=Colors.Black;
		public Color MachineColor
		{
			get { return _machineColor; }
			set { SetProperty(() => _machineColor == value, () => _machineColor = value); }
		}

		private Color _laserOnColor=Colors.Red;
		public Color LaserOnColor
		{
			get { return _laserOnColor; }
			set { SetProperty(() => _laserOnColor == value, () => _laserOnColor = value); }
		}

		private Color _laserOffColor = Colors.Orange;
		public Color LaserOffColor
		{
			get { return _laserOffColor; }
			set { SetProperty(() => _laserOffColor == value, () => _laserOffColor = value); }
		}

		#endregion

		#region GUI-forward

		public class GetLoadInfoArg
		{
			public LoadOptions LoadOption { get; set; }
			public bool UseAzure { get; set; }
		}
		public Func<GetLoadInfoArg, bool> GetLoadInfo { get; set; }

		#endregion

		#region private/intern

		LoadOptions loadinfo = new LoadOptions();
		bool _useAzure = false;
		bool _loadingOrSending = false;

		#endregion

		#region Operations

		public void SendTo()
		{
			new Task(() =>
			{
				_loadingOrSending = true;

				try
				{
					Com.ClearCommandHistory();
					Com.SendCommandsAsync(Commands.ToStringList()).Wait();
					Com.WriteCommandHistory(@"c:\tmp\Command.txt");
				}
				finally
				{
					_loadingOrSending = false;
				}
			}
			).Start();

		}

		public async void Load()
		{
			if (loadinfo.AutoScaleSizeX == 0 || loadinfo.AutoScale == false)
			{
				loadinfo.AutoScaleSizeX = Settings.Instance.SizeX;
				loadinfo.AutoScaleSizeY = Settings.Instance.SizeY;
			}

			var arg = new GetLoadInfoArg() { LoadOption = loadinfo, UseAzure = _useAzure };
			if ((GetLoadInfo?.Invoke(arg)).GetValueOrDefault(false))
			{
				loadinfo = arg.LoadOption;
				_useAzure = arg.UseAzure;

				var ld = new GCodeLoad();

				try
				{
					_loadingOrSending = true;
					Commands = await ld.Load(loadinfo, _useAzure);
				}
				catch (Exception ex)
				{
					MessageBox?.Invoke("Load failed with error: " + ex.Message, "CNCLib", MessageBoxButton.OK, MessageBoxImage.Stop);
				}
				finally
				{
					_loadingOrSending = false;
				}
			}
		}

		public bool CanSendTo()
		{
			return !_loadingOrSending && Com.IsConnected && Commands.Count > 0;
		}

		public bool CanLoad()
		{
			return _loadingOrSending == false;
		}

		public bool CanResetView()
		{
			return true;
		}

		public void ResetView()
		{
			OffsetX = 0;
			OffsetY = 0;
			Zoom = 1;
		}

		#endregion

		#region Commands

		public ICommand LoadCommand { get { return new DelegateCommand(Load, CanLoad); } }
		public ICommand SendToCommand { get { return new DelegateCommand(SendTo, CanSendTo); } }
		public ICommand ResetViewCommand { get { return new DelegateCommand(ResetView, CanResetView); } }

		#endregion
	}
}