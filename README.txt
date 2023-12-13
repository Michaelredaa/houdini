== How to build ==

hcustom -I $HFS/toolkit/include/python2.7 LOP_Sphere.C

or on Windows:

hcustom -I $HFS/python27/include -L $HFS/python27/libs LOP_Sphere.C

These additional include and library search path directories are required
because of the inclusion of USD header files, which in turn include Python
header files.

If the LOP does not show up, 
setenv HOUDINI_DSO_ERROR 1
and look for any dso errors that are reported.

