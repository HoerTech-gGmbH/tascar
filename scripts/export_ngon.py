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

import bpy, bgl, blf, sys, os
from bpy import data, ops, props, types, context

def faceValues(face, mesh, matrix):
  fv = []
  for verti in face.vertices:
    fv.append((matrix * mesh.vertices[verti].co)[:])
  return fv

def faceToLine(face):
    return " ".join([("%.6f %.6f %.6f" % v) for v in face] + ["\n"])

def write(obj,filepath):
  #scene = bpy.context.scene
  faces = []
  me = obj.data
  if me is not None:
    matrix = obj.matrix_world.copy()
    for face in me.tessfaces:
      fv = faceValues(face, me, matrix)
      faces.append(fv)
    for face in me.polygons:
      fv = faceValues(face, me, matrix)
      faces.append(fv)
  # write the faces to a file
  file = open(filepath, "w")
  for face in faces:
      file.write(faceToLine(face))
  file.close()

#
# main:
#
base = os.path.basename(bpy.data.filepath)[0:-6]
for obj in bpy.data.objects:
  if ( obj.type =='MESH'):
    write(obj,base+'_'+obj.name+'.raw')

# Local Variables:
# python-indent-offset: 2
# End:
