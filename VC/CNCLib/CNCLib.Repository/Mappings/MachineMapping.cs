﻿////////////////////////////////////////////////////////
/*
  This file is part of CNCLib - A library for stepper motors.

  Copyright (c) 2013-2015 Herbert Aitenbichler

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

using CNCLib.Repository.Entities;
using System;
using System.Collections.Generic;
using System.Data.Entity.ModelConfiguration;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CNCLib.Repository.Mappings
{
    public class MachineMapping : EntityTypeConfiguration<Machine>
    {
		public MachineMapping()
        {
/*
            this.HasMany(b => b.Authors)
                .WithMany(a => a.Books)
                .Map(x => {
                    x.MapLeftKey("BookId");
                    x.MapRightKey("AuthorId");
                    x.ToTable("AuthorBooks");
                });

            this.HasRequired(b => b.Publisher)
                .WithMany(p => p.Books)
                .HasForeignKey(b => b.PublisherId)
                .WillCascadeOnDelete(false);
 */ 
        }
    }
}
