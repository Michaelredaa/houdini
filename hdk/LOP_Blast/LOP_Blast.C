// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include <cmath>

#include "LOP_Blast.h"
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
        "lop_Blast",               // Internal name
        "Blast",                   // UI name
        LOP_Blast::myConstructor,  // How to build the node
        LOP_Blast::myTemplateList, // parameters
        0,                        // Min # of sources
        1,                        // Max # of sources
        nullptr,                  // Custom local variables (none)
        OP_FLAG_GENERATOR         // Flag it as generator
        );
        op->setIconName("LOP_Blast.png");
    table->addOperator(op);            
}



// Parms
static PRM_Name     primpathName("primpath", "Prim Path");
static PRM_Name     attrnameParmName("attrname", "Attribute");
static PRM_Name     signParmName("sign", "");
static PRM_Name     attrvalParmName("attrval", "");
static PRM_Name     invertParmName("invert", "Invert");

static PRM_Name     signParmTypes[] =
{
    PRM_Name("==", "=="),
    PRM_Name("<", "<"),
    PRM_Name(">", ">"),
    PRM_Name(0)
};
static PRM_ChoiceList   signTypesMenu(PRM_CHOICELIST_SINGLE, signParmTypes);


static PRM_Default  attrnameParmDefault(0.0f, "");
static PRM_Default  attrvalParmDefault(0.0);
static PRM_Default  invertParmDefault = PRM_Default(false);

PRM_Template LOP_Blast::myTemplateList[] = {
    PRM_Template(PRM_STRING,                            1, &lopPrimPathName,         &lopEditPrimPathDefault, &lopPrimPathMenu,0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_STRING | PRM_TYPE_JOIN_NEXT,       1, &attrnameParmName,        &attrnameParmDefault),
    PRM_Template(PRM_ORD | PRM_TYPE_JOIN_NEXT,          1, &signParmName,           0,  &signTypesMenu),
    PRM_Template(PRM_FLT,                               1, &attrvalParmName,         &attrvalParmDefault),
    PRM_Template(PRM_TOGGLE,                            1, &invertParmName,          &invertParmDefault),

    PRM_Template(),
};

OP_Node* LOP_Blast::myConstructor(OP_Network *net, const char *name, OP_Operator *op){
    return new LOP_Blast(net, name, op);
}

LOP_Blast::LOP_Blast(OP_Network *net, const char *name, OP_Operator *op): LOP_Node(net, name, op){
}

LOP_Blast::~LOP_Blast(){}

void LOP_Blast::PRIMPATH(UT_String &str, fpreal t){
    evalString(str, lopPrimPathName.getToken(), 0, t);
    HUSDmakeValidUsdPath(str, true);
}


string LOP_Blast::evalAsString(const PRM_Name& parmName, fpreal t){
    UT_String name;
    evalString(name, parmName.getToken(), 0, t);
    return name.toStdString();
}

OP_ERROR LOP_Blast::cookMyLop(OP_Context &context){
    if (cookModifyInput(context) >= UT_ERROR_FATAL){return error();}

    fpreal now = context.getTime();
    UT_String primpath;
    PRIMPATH(primpath, now);

    string attrname = evalAsString(attrnameParmName, now);
    string sign = evalAsString(signParmName, now);
    fpreal attrval = evalFloat(attrvalParmName.getToken(), 0, now);
    int invert = bool(evalInt(invertParmName.getToken(), 0, now));

    return error();

}