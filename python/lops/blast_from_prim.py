import numpy as np
from pxr import Usd, Sdf, UsdGeom, Vt

node = hou.pwd()
stage = node.editableStage()

# Add code to modify the stage.
# Use drop down menu to select examples.

prim = stage.GetPrimAtPath("/senece/mountain/height")
mesh = UsdGeom.Mesh(prim)

# Mask
mask_attr = prim.GetAttribute("primvars:mask")
mask_values = np.array(mask_attr.Get())
pts_num = np.where(mask_values > 0.5)[0]
pts_num = np.sort(pts_num)[::-1]

# Mesh
points_attr = mesh.GetPointsAttr()
fv_indices_attr = mesh.GetFaceVertexIndicesAttr()
fv_counts_attr = mesh.GetFaceVertexCountsAttr()

# Values
points_values = np.array(points_attr.Get())
fv_indices = np.array(fv_indices_attr.Get())
fv_counts = np.array(fv_counts_attr.Get())

# Delete mask
# points
points_values = np.delete(points_values, pts_num, axis=0)

# Faces
faces = fv_indices.reshape(-1, 4)
non_deleted_faces = faces[~np.any(np.isin(faces, pts_num), axis=1)]
deleted_faces_indices = np.where(np.any(np.isin(faces, pts_num), axis=1))[0]
fv_counts = np.delete(fv_counts, deleted_faces_indices, axis=0)

# FaceVertex
fv_indices = non_deleted_faces.flatten()
deleted_fv_indices = []
for i in deleted_faces_indices:
    for c in range(4):
        deleted_fv_indices.append((i * 4) + c)

for num in pts_num:
    fv_indices = np.where(fv_indices > num, fv_indices - 1, fv_indices)

# Update attribute values
points_attr.Set(Vt.Vec3fArray.FromNumpy(points_values))
fv_indices_attr.Set(Vt.IntArray.FromNumpy(fv_indices))
fv_counts_attr.Set(Vt.IntArray.FromNumpy(fv_counts))

for attr in prim.GetAttributes():
    if attr.GetName() in ['points', 'faceVertexIndices', 'faceVertexCounts']:
        continue

    attr_type_name = attr.GetTypeName()
    attr_value = attr.Get()

    interpolation = attr.GetMetadata('interpolation')
    if not interpolation:
        continue

    if len(attr.Get()) < 1:
        continue

    class_type = type(attr.Get())

    if interpolation == 'vertex':
        verties_values = np.array(attr.Get())
        new_verties_values = np.delete(verties_values, pts_num, axis=0)
        attr.Set(class_type.FromNumpy(new_verties_values))

    if interpolation == 'uniform':
        uniform_values = np.array(attr.Get())
        new_uniform_values = np.delete(uniform_values, deleted_faces_indices, axis=0)
        attr.Set(class_type.FromNumpy(new_uniform_values))

    if interpolation == 'faceVarying':
        fv_values = np.array(attr.Get())
        new_fv_values = np.delete(fv_values, deleted_fv_indices, axis=0)
        attr.Set(class_type.FromNumpy(new_fv_values))


