#!/bin/sh
# *********************************************************************************** #
#  mk - Copy sources and run build on target system                                   #
# *********************************************************************************** #
#                                                                                     #
#  Copyright (c) 2020 by Bodo Bauer <bb@bb-zone.com>                                  #
#                                                                                     #
#  This program is free software: you can redistribute it and/or modify               #
#  it under the terms of the GNU General Public License as published by               #
#  the Free Software Foundation, either version 3 of the License, or                  #
#  (at your option) any later version.                                                #
#                                                                                     #
#  This program is distributed in the hope that it will be useful,                    #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of                     #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      #
#  GNU General Public License for more details.                                       #
#                                                                                     #
#  You should have received a copy of the GNU General Public License                  #
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.              #
# *********************************************************************************** #
build_host=192.168.100.50
build_dir=valveControl

source_files="*.c *.h CMakeLists.txt valvecontrol.cnf valvecontrol"

scp $source_files $build_host:$build_dir
ssh -x $build_host "(cd $build_dir && cmake . && make)"
