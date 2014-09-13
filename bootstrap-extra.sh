#!/bin/sh
#
# Copyright 2012 Peter Schueller <schueller.p@gmail.com>
#
# This extends bootstrap.sh by additional features required for shipping dlvhex
# (in particular we might want to ship parts of the potassco release within dlvhex)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CLASPFNAME=clasp-3.1.0-source.tar.gz
if [ ! -e $CLASPFNAME ]; then
  echo "downloading $CLASPFNAME"
  wget http://downloads.sourceforge.net/project/potassco/clasp/3.1.0/$CLASPFNAME
fi

GRINGOFNAME=gringo-4.4.0-source.tar.gz
if [ ! -e $GRINGOFNAME ]; then
  echo "downloading $GRINGOFNAME"
  wget http://downloads.sourceforge.net/project/potassco/gringo/4.4.0/$GRINGOFNAME
fi

if [ ! -e $CLASPFNAME ] || [ ! -e $GRINGOFNAME ]; then
  echo "could not download/find clasp or gringo source archives!"
  exit 1
fi

exit 0
