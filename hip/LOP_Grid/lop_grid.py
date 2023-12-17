# -*- coding: utf-8 -*-
"""
Documentation: This script used inside python node to create a usd geom as a grid
"""

import math

import hou

from pxr import Usd, UsdGeom, Gf

node = hou.pwd()
stage = node.editableStage()

prim_path = node.parm("primpath").eval()
orientation = node.parm("orientation").evalAsString()
size = node.parmTuple("size").eval()
center = node.parmTuple("center").eval()
rotate = node.parmTuple("rotate").eval()

rows = node.parm("rows").eval()
columns = node.parm("columns").eval()

if rows < 2:
    rows = 2

if columns < 2:
    columns = 2


def rotate_point(point: list, angle: list) -> tuple:
    x, y, z = point
    qx, qy, qz = point

    # @X
    if angle[0] != 0:
        rad = math.radians(angle[0])
        qz = z * math.cos(rad) - y * math.sin(rad)
        qy = z * math.sin(rad) + y * math.cos(rad)
        x, y = qx, qy

    # @Y
    if angle[1] != 0:
        rad = math.radians(angle[1])
        qx = x * math.cos(rad) + z * math.sin(rad)
        qz = -x * math.sin(rad) + z * math.cos(rad)
        x, y = qx, qy

    # @Z
    if angle[2] != 0:
        rad = math.radians(angle[2])
        qx = x * math.cos(rad) - y * math.sin(rad)
        qy = x * math.sin(rad) + y * math.cos(rad)

    return qx, qy, qz


def create_grid(
        prim_path: str,
        rows: int,
        columns: int,
        size: list[float],
        orientation: str,
        center: list[float],
        rotate: list[float],

) -> Usd.UsdGeom.Mesh:
    # Add Mesh
    mesh = UsdGeom.Mesh.Define(stage, prim_path)

    # Add points
    points = []

    for r in range(rows):
        for c in range(columns):
            x = (r / (rows - 1) - 0.5) * size[0]
            y = 0
            z = (c / (columns - 1) - 0.5) * size[1]

            point = (x, y, z)

            if orientation == 'xy':
                point = (y, x, z)

            if orientation == 'yz':
                point = (y, x, z)

            point = rotate_point(point, rotate)

            point = [(point[i] + center[i]) + point[i] for i in range(3)]

            points.append(point)

        # Face Vertrics Count
    vertic_count = [4] * (rows - 1) * (columns - 1)
    mesh.CreateFaceVertexCountsAttr(vertic_count)

    # Create Faces

    vertex_indices = []
    for r in range(rows - 1):
        for c in range(columns - 1):
            idx = r * columns + c
            face = [idx, idx + 1, idx + 1 + columns, idx + columns]
            vertex_indices.extend(face)

    mesh.CreateFaceVertexIndicesAttr(vertex_indices)

    mesh.CreatePointsAttr(points)

    return mesh


create_grid(prim_path, rows, columns, size, orientation, center, rotate)

if __name__ == '__main__':
    print(__name__)
