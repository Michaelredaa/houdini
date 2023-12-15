
#ifndef __LOP_Grid_h__
#define __LOP_Grid_h__

#include <LOP/LOP_Node.h>

namespace HOU_HDK {

class LOP_Grid: public LOP_Node
{
public:
	LOP_Grid(OP_Network *net, const char *name, OP_Operator *op);
    ~LOP_Grid() override;

    static PRM_Template	 myTemplateList[];
    static OP_Node	*myConstructor(OP_Network *net, const char *name, OP_Operator *op);

protected:
    /// Method to cook USD data for the LOP
    OP_ERROR cookMyLop(OP_Context &context) override;

    UT_Vector3 getXYZParameterValue(const PRM_Name& paramName, fpreal t);


private:
    void PRIMPATH(UT_String&str, fpreal t);

};

} // End HOU_HDK namespace

#endif
