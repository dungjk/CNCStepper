﻿////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2014 Herbert Aitenbichler

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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Globalization;
using Framework.Tools;

namespace GCode.Logic.Commands
{
    public abstract class Command
	{
		public enum MoveType
		{
			NoMove,
			Fast,		// Go
			Normal		// G1,G2
		};

		#region crt

		public Command()
        {
			PositionValid = false;
			Movetype = MoveType.NoMove;
        }

		private SpaceCoordinate _CalculatedEndPosition;
		protected List<Variable> _variables = new List<Variable>();

		#endregion

        #region Property

		public Command NextCommand { get; set; }
		public Command PrevCommand { get; set; }

        public SpaceCoordinate CalculatedStartPosition 
		{
			get
			{
				return PrevCommand == null ? new SpaceCoordinate() : PrevCommand.CalculatedEndPosition;
			}
		}
		public SpaceCoordinate CalculatedEndPosition
		{
			get
			{
				return _CalculatedEndPosition;
			}
		}			

		public bool PositionValid { get; protected set; }
		public MoveType Movetype { get; protected set; }

		public string SubCode { get; protected set;  }
		public string Code { get; protected set; }

		protected class Variable
		{
			public char Name;
			public decimal Value;
			public string Parameter;

			public string ToGCode() 
			{ 
				if (string.IsNullOrEmpty(Parameter))
					return Name + Value.ToString(CultureInfo.InvariantCulture); 
				return Name + "#" + Parameter; 
			}
		};

		public void AddVariable(char name, decimal value)
		{
			_variables.Add(new Variable() { Name = name, Value = value });
		}
		public void AddVariableParam(char name, string paramvalue)
		{
			_variables.Add(new Variable() { Name = name, Parameter = paramvalue });
		}

		public decimal GetVariable(char name, decimal defaultvalue)
		{
			decimal ret;
			if (TryGetVariable(name, out ret))
				return ret;
			return defaultvalue;
		}

		public bool TryGetVariable(char name, out decimal val)
		{
			Variable var = _variables.Find( n => n.Name == name);
			if (var!=null)
			{
				val = var.Value;
				return true;
			}
			val = 0;
			return false;
		}

        #endregion

        #region Draw

		public virtual void Draw(IOutputCommand output, object param)
		{
			output.DrawLine(this, param, Movetype, CalculatedStartPosition, CalculatedEndPosition);
		}
		
		#endregion

		#region GCode

		public string GCodeAdd { get; set; }

		protected string GCodeHelper(SpaceCoordinate current)
        {
			String cmd = Code;
			if (!string.IsNullOrEmpty(SubCode))
				cmd += "." + SubCode;

			foreach (Variable p in _variables )
			{
				cmd += " " + p.ToGCode();
			}

			if (!string.IsNullOrEmpty(GCodeAdd))
			{
				if (!string.IsNullOrEmpty(cmd))
					cmd += " ";

				cmd += GCodeAdd;
			}
			return cmd;
        }

		public virtual string[] GetGCodeCommands(SpaceCoordinate startfrom)
		{
			string[] ret = new string[] 
            {
                GCodeHelper(startfrom)
            };
			return ret;
		}

		#endregion

		#region Serialisation

		public void UpdateCalculatedEndPosition()
		{
			if (PositionValid)
			{
				SpaceCoordinate sc = new SpaceCoordinate();
				decimal val;

				if (TryGetVariable('X', out val)) sc.X = val;
				if (TryGetVariable('Y', out val)) sc.Y = val;
				if (TryGetVariable('Z', out val)) sc.Z = val;
				if (!sc.HasAllValues && PrevCommand != null)
				{
					sc.AssignMissing(PrevCommand.CalculatedEndPosition);
				}
				_CalculatedEndPosition = sc;;
			}
			else
			{
				_CalculatedEndPosition = (PrevCommand == null) ? new SpaceCoordinate() : PrevCommand._CalculatedEndPosition;
			}
		}			

		private void ReadFromToEnd(CommandStream stream)
		{
			GCodeAdd = "";
			while (!stream.IsEOF())
			{
				GCodeAdd += stream.NextChar;
				stream.Next();
			}
		}

		protected decimal ReadVariable(CommandStream stream, char param)
		{
			stream.Next();
			stream.SkipSpaces();
			if (stream.NextChar == '#')
			{
				stream.Next();
				int paramNr = stream.GetInt();
				AddVariableParam(param,paramNr.ToString());
				return 0;
			}
			decimal val = stream.GetDecimal() ;
			AddVariable(param,val);
			return val;
		}

		public virtual bool ReadFrom(CommandStream stream)
		{
			SpaceCoordinate ep = new SpaceCoordinate();

			if (stream.NextChar == '.')
			{
				stream.Next();
				SubCode = stream.GetInt().ToString();
			}

			if (PositionValid)
			{
				while (true)
				{
					switch (stream.SkipSpacesToUpper())
					{
						case 'X': ep.X = ReadVariable(stream, stream.NextCharToUpper); break;
						case 'Y': ep.Y = ReadVariable(stream, stream.NextCharToUpper); break;
						case 'Z': ep.Z = ReadVariable(stream, stream.NextCharToUpper); break;
						case 'F':
						case 'R':
						case 'I':
						case 'J':
						case 'K': ReadVariable(stream,stream.NextCharToUpper); break;
						default:
							{
								ReadFromToEnd(stream);
								return true;
							}
					}
				}
			}
			else
			{
				ReadFromToEnd(stream);
			}

			return true;
		}

		#endregion
	}
}