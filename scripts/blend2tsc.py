import bpy, bgl, blf, sys, os
from bpy import data, ops, props, types, context

def faceToTriangles(face):
  triangles = []
  if len(face) == 4:
    triangles.append([face[0], face[1], face[2]])
    triangles.append([face[2], face[3], face[0]])
  else:
    triangles.append(face)
  return triangles


def faceValues(face, mesh, matrix):
  fv = []
  for verti in face.vertices:
    fv.append((matrix * mesh.vertices[verti].co)[:])
  return fv


def faceToLine(face):
  return " ".join([("%.6f %.6f %.6f" % v) for v in face] + ["\n"])


def write(filepath,
          applyMods=True,
          triangulate=True,
          ):

    scene = bpy.context.scene

    faces = []
    for obj in bpy.context.selected_objects:
        if applyMods or obj.type != 'MESH':
            try:
                me = obj.to_mesh(scene, True, "PREVIEW")
            except:
                me = None
            is_tmp_mesh = True
        else:
            me = obj.data
            if not me.tessfaces and me.polygons:
                me.calc_tessface()
            is_tmp_mesh = False

        if me is not None:
            matrix = obj.matrix_world.copy()
            for face in me.tessfaces:
                fv = faceValues(face, me, matrix)
                if triangulate:
                    faces.extend(faceToTriangles(fv))
                else:
                    faces.append(fv)

            if is_tmp_mesh:
                bpy.data.meshes.remove(me)

    # write the faces to a file
    file = open(filepath, "w")
    for face in faces:
        file.write(faceToLine(face))
    file.close()

def obj_faces(obj):
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
  return faces
  #for face in faces:
  #    file.write(faceToLine(face))


#
# main:
#
base = os.path.basename(bpy.data.filepath)[0:-6]
fh = open(base+'_export.tsc','w')
rad2deg = 57.295779513082322865
for obj in bpy.data.objects:
  if ( obj.type =='MESH'):
    if ('reflectivity' in obj):
      # this is a reflector mesh:
      fh.write('<facegroup name="'+obj.name+('" reflectivity="%g"' % obj['reflectivity']))
      if ('damping' in obj):
        fh.write(' damping="%g"' % obj['damping'])
      fh.write('>\n<faces>')
      faces = obj_faces(obj)
      for face in faces:
        fh.write(faceToLine(face))
      fh.write('</faces>\n</facegroup>\n')
    if ('source' in obj):
      fh.write('<src_object name="'+obj.name+'">\n')
      fh.write('<position>0 '+('%g %g %g' % obj.location[:])+'</position>\n')
      fh.write('<orientation>0 '+('%g ' % (obj.rotation_euler[2]*rad2deg))+('%g ' % (obj.rotation_euler[1]*rad2deg))+('%g ' % (obj.rotation_euler[0]*rad2deg))+'</orientation>\n')
      k = 0
      for vert in obj.data.vertices:
        fh.write(('<sound name="%d" ' % k)+('connect="@.0.%d" ' % k))
        fh.write('x="%g" y="%g" z="%g"/>\n' % vert.co[:])
        k = k+1
      fh.write('<sndfile name="'+obj['source']+('" channels="%d"' % k)+'/>\n')
      fh.write('</src_object>\n')
    #write(base+'_'+obj.name+'.raw')
    #obj.select = False
    #out = open(base+'_'+obj.name+'.cmd', 'w')
    #out.write('filev '+base+'_'+obj.name+'.cn %f 1\n' % obj['cnc'])
fh.close()

# Local Variables:
# python-indent-offset: 2
# End:
