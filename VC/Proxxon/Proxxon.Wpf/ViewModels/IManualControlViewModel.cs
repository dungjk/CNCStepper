﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Proxxon.Wpf.ViewModels
{
	public interface IManualControlViewModel
	{
		void AsyncRunCommand(Action todo);
		void SetPositions(string[] positions);
	}
}
