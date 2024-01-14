// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include <cmath>

#include "LOP_Noise.h"
#include <LOP/LOP_PRMShared.h>
#include <LOP/LOP_Error.h>
#include <HUSD/HUSD_CreatePrims.h>
#include <HUSD/HUSD_Constants.h>
#include <HUSD/HUSD_Utils.h>
#include <HUSD/XUSD_Data.h>
#include <HUSD/XUSD_Utils.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Operator.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Noise.h>
#include <UT/UT_NoiseBasis.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>


using namespace HOU_HDK;
using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE


void newLopOperator(OP_OperatorTable *table){

    OP_Operator *op = new OP_Operator(
        "lop_Noise",               // Internal name
        "Noise",                   // UI name
        LOP_Noise::myConstructor,  // How to build the node
        LOP_Noise::myTemplateList, // parameters
        0,                        // Min # of sources
        1,                        // Max # of sources
        nullptr,                  // Custom local variables (none)
        OP_FLAG_GENERATOR         // Flag it as generator
        );
        op->setIconName("LOP_Noise.png");
    table->addOperator(op);            
}

// Parms
static PRM_Name primpathName("primpath", "Prim Path");

static PRM_Name     noisetypesParmName("noisetype", "Noise Type");
static PRM_Name noiseTypes[] =
{
    PRM_Name("worly", "Worly"),
    PRM_Name(0)
};

static PRM_ChoiceList   noiseTypesMenu(PRM_CHOICELIST_SINGLE, noiseTypes);
static PRM_Name     frequencyParmName("frequency", "Frequency");
static PRM_Default  frequencyParamDefault[] = {PRM_Default(5.0), PRM_Default(5.0), PRM_Default(5.0)};
static PRM_Range    frequencyParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 100);

static PRM_Name     amplitudeParmName("amplitude", "Amplitude");
static PRM_Default  amplitudeParamDefault = PRM_Default(0.5);
static PRM_Range    amplitudeParamRange(PRM_RANGE_UI , 0, PRM_RANGE_UI , 10);

static PRM_Name     seedParmName("seed", "Seed");
static PRM_Default  seedParamDefault = PRM_Default(0);
static PRM_Range    seedParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 10);

static PRM_Name     offsetParmName("offset", "Offset");
static PRM_Default  offsetParamDefault[] = {PRM_Default(0.0), PRM_Default(0.0), PRM_Default(0.0)};
static PRM_Range    offsetParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 100);



// PRM_Template(int type, int vector_size, const PRM_Name *name, const PRM_Default *def, unsigned int export_flag = 0, const PRM_SpareData *spare = 0, const PRM_Callback *cb = 0);
PRM_Template
LOP_Noise::myTemplateList[] = {
    PRM_Template(PRM_STRING,       1, &lopPrimPathName,     &lopAddPrimPathDefault, &lopPrimPathMenu,0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_ORD,          1, &noisetypesParmName,                                  0,  &noiseTypesMenu     ),
    PRM_Template(PRM_XYZ_J,        3, &frequencyParmName,      frequencyParamDefault,       0,  &frequencyParamRange),
    PRM_Template(PRM_FLT,          1, &amplitudeParmName,     &amplitudeParamDefault,       0,  &amplitudeParamRange),
    PRM_Template(PRM_INT,          1, &seedParmName,               &seedParamDefault,       0,  &seedParamRange     ),
    PRM_Template(PRM_XYZ_J,        3, &offsetParmName,            offsetParamDefault,       0,  &offsetParamRange   ),
    PRM_Template(),
};

OP_Node* LOP_Noise::myConstructor(OP_Network *net, const char *name, OP_Operator *op){
    return new LOP_Noise(net, name, op);
}

LOP_Noise::LOP_Noise(OP_Network *net, const char *name, OP_Operator *op): LOP_Node(net, name, op){
}

LOP_Noise::~LOP_Noise(){
}

void LOP_Noise::PRIMPATH(UT_String &str, fpreal t){
    evalString(str, lopPrimPathName.getToken(), 0, t);
    HUSDmakeValidUsdPath(str, true);
}


UT_Vector3 LOP_Noise::getXYZParameterValue(const PRM_Name& parmName, fpreal t)
{
    UT_Vector3 value(0.0, 0.0, 0.0);

    for (int i = 0; i < 3; ++i)
    {
        fpreal componentValue = evalFloat(parmName.getToken(), i, t);
        value[i] = componentValue;
    }

    return value;
}


string LOP_Noise::evalOrdAsString(const PRM_Name& parmName, fpreal t){
    UT_String name;
    evalString(name, parmName.getToken(), 0, t);
    return name.toStdString();
}



OP_ERROR LOP_Noise::cookMyLop(OP_Context &context){
    // Cook the node connected to our input, and make a "soft copy" of the
    // result into our own HUSD_DataHandle.
    if (cookModifyInput(context) >= UT_ERROR_FATAL)
	return error();

    fpreal now = context.getTime();

    UT_String primpath;
    fpreal amplitude = evalFloat(amplitudeParmName.getToken(), 0, now);
    int32 seed = evalInt(seedParmName.getToken(), 0, now);

    UT_Vector3 frequency = getXYZParameterValue(frequencyParmName, now);
    UT_Vector3 offset = getXYZParameterValue(offsetParmName, now);

    string noisetype = evalOrdAsString(noisetypesParmName, now);



    PRIMPATH(primpath, now);

    // Use editableDataHandle to get non-const access to our data handle, and
    // the lock it for writing. This makes sure that the USD stage is set up
    // to match the configuration defined in our data handle. Any edits made
    // to the stage at this point will be preserved in our data handle when
    // we unlock it.
    HUSD_AutoWriteLock writelock(editableDataHandle());
    HUSD_AutoLayerLock layerlock(writelock);

    // Create a helper class for creating USD primitives on the stage.
    HUSD_CreatePrims creator(layerlock);

    // Use the helper class to create a new "Sphere" primitive, with a
    // specifier type of "def" to define this primitive even if it doesn't
    // exist on the stage yet.
    //
    if (!creator.createPrim(primpath, "",
            UT_StringHolder::theEmptyString,
            HUSD_Constants::getPrimSpecifierOverride(),
            HUSD_Constants::getGeomGprimPrimType()))
    {
        addError(LOP_PRIM_NOT_FOUND, primpath);
        return error();
    }


    // Use direct editing of the USD stage to modify the radius attribute
    // of the new sphere primitive. We could use the HUSD_SetAttributes
    // helper class here, but this demonstrates that we are allowed to
    // access the stage directly. The edit target will already be set to
    // the "active layer", and as long as we leave it there, any changes
    // we make will be preserved by our data handle.
    UsdStageRefPtr stage = writelock.data()->stage();
    UsdPrim prim = stage->GetPrimAtPath(SdfPath(HUSDgetSdfPath(primpath)));

    VtArray<GfVec3f> points;

    UsdGeomMesh mesh(prim);
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    pointsAttr.Get(&points, now);

    // using namespace UT_Noise;


    /*
    int seed;
    float f1, f2, f3, f4;

    int speed = chi("speed");
    float shift = ch("shift")*speed;
    vector pos = @P * chf("frequency");
    vector4 v = set(pos.x, pos.y, pos.z, shift);

    // wnoise(vector4 position, int &seed, float &f1, float &f2, float &f3, float &f4, int periodx, int periody, int periodz, int periodw)

    wnoise(v, seed, f1, f2, f3, f4, 0, 0, 0, speed);

    float wval = f1+f2+f3+f4;
    @P.y += wval*ch("scale");
    f@seed = seed;
    
    */

    VtArray<int> patchs;
    for(int i=0; i<points.size(); i++){

        // static void 	noise4D (const fpreal32 P[4], const fpreal32 *jitter, int32 *period, int32 &seed, fpreal32 &f1, fpreal32 &f2, fpreal32 p0[4], fpreal32 p1[4])        // Example of 1D noise
        UT_NoiseBasis noiseBasis;
        noiseBasis.initNoise();

        fpreal32 jitter = 0.1;   // Example jitter value
        int32 period = 0;    // Example period value
        int32 patch = 0;

        fpreal32 f1, f2;
        fpreal32 p0[3], p1[3];

        GfVec3f pos = points[i];
        pos = pos * frequency[0];

        fpreal32 p[2] = {
            pos[0],
            pos[2]
            };

        noiseBasis.noise3D(p, &jitter, &period, patch, f1, f2, p0, p1);

        // points[i][1] += (f1 + f2 + p0[1] + p1[1])*amplitude;
        points[i][1] += (f1 + f2)*amplitude;

        patchs.emplace_back(patch);

        // UT_Noise noise(seed, UT_Noise::SPARSE);
        // UT_Vector3 p = {points[i][0], points[i][1], points[i][2]};

        //  turbulence (const UT_Vector3F &pos, unsigned fractalDepth, fpreal rough=0.5, fpreal atten=1.0) const
        // fpreal noiseValue = noise.turbulence(p, frequency[0], frequency[1], offset[0]) * amplitude;

        // points[i][1] += noiseValue;
    }

    pointsAttr.Set(points);
    prim.CreateAttribute(TfToken("primvars:patch"), SdfValueTypeNames->IntArray).Set(patchs);

    return error();
}

