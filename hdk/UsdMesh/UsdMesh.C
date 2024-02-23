class UsdGeo : public UsdGeomMesh {
public:
    UsdGeo(const UsdPrim& prim) : UsdGeomMesh(prim) {
        // Initialize member attributes in the constructor
        pointsAttr.Get(&points);
        FVCAttr.Get(&fv_count);
        FVIAttr.Get(&fv_indices);
    }


    VtArray<VtIntArray> GetFaces(){
        VtArray<VtIntArray>  faces;
        int fv = 0;

        for(int f=0; f<fv_count.size(); f++){
            int end_idx = fv + fv_count[f];

            VtIntArray face(fv_indices.begin() + fv, fv_indices.begin() + end_idx);
            faces.push_back(face);
            fv = end_idx;
        }
        return faces;
    }



    VtArray<int> GetConnectedPoints(int pointIndex) {
        // Implementation of GetConnectedPoints goes here
        // You can use 'points', 'fv_count', and 'fv_indices' here
        // ...

        VtArray<int> connectedPoints;
        // Populate connectedPoints based on the logic

        return connectedPoints;
    }

private:
    VtArray<GfVec3f> points;
    VtArray<int> fv_count;
    VtArray<int> fv_indices;

    UsdAttribute pointsAttr = GetPointsAttr();
    UsdAttribute FVCAttr = GetFaceVertexCountsAttr();
    UsdAttribute FVIAttr = GetFaceVertexIndicesAttr();
};
