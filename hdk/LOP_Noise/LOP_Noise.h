
#ifndef __LOP_Noise_h__
#define __LOP_Noise_h__

#include <LOP/LOP_Node.h>

namespace HOU_HDK {

class LOP_Noise: public LOP_Node
{
public:
	LOP_Noise(OP_Network *net, const char *name, OP_Operator *op);
    ~LOP_Noise() override;

    static PRM_Template	 myTemplateList[];
    static OP_Node	*myConstructor(OP_Network *net, const char *name, OP_Operator *op);

protected:
    /// Method to cook USD data for the LOP
    OP_ERROR cookMyLop(OP_Context &context) override;

    UT_Vector3 getXYZParameterValue(const PRM_Name& parmName, fpreal t);
    std::string evalOrdAsString(const PRM_Name& parmName, fpreal t);


private:
    void PRIMPATH(UT_String&str, fpreal t);

};

} // End HOU_HDK namespace

#endif
