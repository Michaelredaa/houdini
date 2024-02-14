#ifndef __LOP_Blast_h__
#define __LOP_Blast_h__

#include <LOP/LOP_Node.h>

namespace HOU_HDK
{
    class LOP_Blast: public LOP_Node{

        public:
            LOP_Blast(OP_Network *net, const char *name, OP_Operator *op);
            ~LOP_Blast() override;

            static PRM_Template	 myTemplateList[];
            static OP_Node	*myConstructor(OP_Network *net, const char *name, OP_Operator *op);

        protected:
            /// Method to cook USD data for the LOP
            OP_ERROR cookMyLop(OP_Context &context) override;

            UT_Vector3 getXYZParameterValue(const PRM_Name& parmName, fpreal t);
            std::string evalAsString(const PRM_Name& parmName, fpreal t);

        private:
            void PRIMPATH(UT_String&str, fpreal t);

    };



} // namespace HOU_HDK


#endif