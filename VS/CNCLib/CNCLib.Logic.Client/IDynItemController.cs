﻿////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2017 Herbert Aitenbichler

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
using System.Threading.Tasks;
using CNCLib.Logic.Contracts.DTO;

namespace CNCLib.Logic.Client
{
	public interface IDynItemController : IDisposable
	{
		Task<DynItem> Get(int id);

		Task<IEnumerable<DynItem>> GetAll();

		Task<IEnumerable<DynItem>> GetAll(Type t);

		Task<object> Create(int id);

        Task<int> Add(string name, object value);
        Task Save(int id, string name, object value);
        Task Delete(int id);
    }
}