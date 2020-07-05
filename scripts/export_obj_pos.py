# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

import bpy, bgl, blf, sys, os, math
from bpy import data, ops, props, types, context

#
# main:
#
oname = os.getenv('TSCOBJ')
base = os.path.basename(bpy.data.filepath)[0:-6]
tsc = bpy.context.scene
file = open('objpos.txt', "w")
for obj in bpy.data.objects:
  if (tsc in obj.users_scene):
    if( oname == obj.name ):
      print(obj.name)
      obj.rotation_mode = 'ZYX'
      print('<position>0 ',math.degrees(obj.rotation_euler.z))
      file.write( '<position>0 %1.3f %1.3f %1.3f</position>\n' % (obj.location.x,
                                                                  obj.location.y,
                                                                  obj.location.z) )
      file.write( '<orientation>0 %1.1f %1.1f %1.1f</orientation>\n' % (math.degrees(obj.rotation_euler.z),
                                                                        math.degrees(obj.rotation_euler.y),
                                                                        math.degrees(obj.rotation_euler.x)) )
      file.write( 'x="%1.3f" y="%1.3f" z="%1.3f"\n' % (obj.location.x,
                                                       obj.location.y,
                                                       obj.location.z) )
file.close()

# Local Variables:
# python-indent-offset: 2
# End:
