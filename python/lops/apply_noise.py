# -*- coding: utf-8 -*-
"""
Documentation:
"""

from pxr import Usd, UsdGeom, Sdf, Vt, Gf
import numpy as np
from opensimplex import OpenSimplex

def convert_vt_to_np(array: Vt.Vec3fArray) -> np.ndarray:
    return np.array(array)


def convert_np_to_vt(array: np.ndarray) -> Vt.Vec3fArray:
    return Vt.Vec3fArray.FromNumpy(array)

def add_noise(geom_layer, noise_layer):
    stage = Usd.Stage.Open(geom_layer)
    new_stage = Usd.Stage.CreateNew(noise_layer)

    grid_path = Sdf.Path("/grid")
    prim = stage.GetPrimAtPath(grid_path)
    geom_api = UsdGeom.Mesh(prim)
    points_attr = geom_api.GetPointsAttr()

    points = convert_vt_to_np(points_attr.Get())

    # Add Noise
    freq = 5
    amplitude = 0.3
    for ptn in points:
        noise = OpenSimplex(10)
        value = noise.noise3(*ptn * freq)

        ptn[1] += value * amplitude

    over_grid_prim = new_stage.OverridePrim(grid_path)
    over_grid_prim.CreateAttribute("points", Sdf.ValueTypeNames.Point3fArray).Set(convert_np_to_vt(points))
    new_stage.GetRootLayer().Export(noise_layer)


geom_layer = r"C:\Users\michael\Documents\work\houdini\usd\grid\geom.usd"
noise_layer = r'C:\Users\michael\Documents\work\houdini\usd\grid\noise.usda'
laplace_layer = r'C:\Users\michael\Documents\work\houdini\usd\grid\laplace.usda'

# add_noise(geom_layer, noise_layer)



stage = Usd.Stage.CreateInMemory()

stage.GetRootLayer().subLayerPaths = [noise_layer, geom_layer]
grid_path = Sdf.Path("/grid")
prim = stage.GetPrimAtPath(grid_path)
geom_api = UsdGeom.Mesh(prim)

points_attr = geom_api.GetPointsAttr()
points_fv_count_attr = geom_api.GetFaceVertexCountsAttr()
points_fv_indices_attr = geom_api.GetFaceVertexIndicesAttr()

y_attr = prim.CreateAttribute("primvars:y", Sdf.ValueTypeNames.FloatArray)
y_attr.SetMetadata('interpolation', "vertex")
laplace_attr = prim.CreateAttribute("primvars:laplace", Sdf.ValueTypeNames.FloatArray)
laplace_attr.SetMetadata('interpolation', "vertex")

points = convert_vt_to_np(points_attr.Get())

zeros = np.zeros(points.shape[0])

if not y_attr.HasValue():
    y_attr.Set(zeros, -1)

if not laplace_attr.HasValue():
    laplace_attr.Set(zeros, -1)

propagation = 0.01


points_fv_count = convert_vt_to_np(points_fv_count_attr.Get())
points_fv_indices = convert_vt_to_np(points_fv_indices_attr.Get())


for t in range(300):
    # Get laplace
    points = convert_vt_to_np(geom_api.GetPointsAttr().Get(t - 1))
    y_values = y_attr.Get(t - 1)

    faces = []
    fv = 0
    for i in range(points_fv_count.size):
        face = points_fv_indices[fv: fv + points_fv_count[i]]
        faces.append(face)
        fv += points_fv_count[i]
    faces = np.array(faces)

    laplace_values = []
    for i, ptn in enumerate(points):
        laplace = 0
        connected_faces = faces[np.any(faces == i, axis=1)]
        for connected_face in connected_faces:
            for connected_ptn in connected_face:
                if connected_ptn == i:
                    continue
                laplace += points[connected_ptn][1] - ptn[1]
        laplace_values.append(laplace)

    laplace_values = np.array(laplace_values) * propagation
    y_values = np.add(y_values, laplace_values)

    result = points.copy()
    points[:, 1] += y_values

    y_attr.Set(y_values, t)
    points_attr.Set(convert_np_to_vt(points), t)
    laplace_attr.Set(laplace_values, t)

"""
vector ptn_pos;

foreach(int pt_num; neigh_ptns){

    // connected
    ptn_pos = pointattrib(0, "P", pt_num, 1);
    laplace +=  (ptn_pos.y - v@P.y) *ch("connected_weight");
    
    // corners
    foreach(int prim; prims){
        int points[] = primpoints(0, prim);
            foreach(int ptn; points){
                int found = find(neigh_ptns, ptn);
                // printf("%i\n", found);
                if (find(neigh_ptns, ptn) < 0){
                        ptn_pos = pointattrib(0, "P", pt_num, 1);
                        laplace +=  (ptn_pos.y - v@P.y)*ch("corner_weight");
                }
            }
    }
    
} 

f@laplace = ch("propagation") * laplace;

f@y += @laplace;
@P.y += @y;

"""

stage.GetRootLayer().subLayerPaths
Sdf.Layer.CreateAnonymous(tag="ripples")

for layer in Sdf.Layer.GetLoadedLayers():
    if layer.anonymous:
        layer.GetPrimAtPath("").GetPrimAtPath("").path