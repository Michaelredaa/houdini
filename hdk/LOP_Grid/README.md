# == How to build ==
## Linux:
`hcustom -I $HFS/toolkit/include/python2.7 LOP_Grid.C`

## Windows:

`hcustom -I $HFS/python37/include -L $HFS/python37/libs LOP_Grid.C`

These additional include and library search path directories are required
because of the inclusion of USD header files, which in turn include Python
header files.

If the LOP does not show up, 
`setenv HOUDINI_DSO_ERROR 1`

and look for any dso errors that are reported.

# For me was run on windows like:
```
cd Documents\houdini19.5\dso
"C:\Program Files\Side Effects Software\Houdini 19.5.303\bin\hcustom.exe" -I "C:\Program Files\Side Effects Software\Houdini 19.5.303\python37\include" -L "C:\Program Files\Side Effects Software\Houdini 19.5.303\python37\libs" houdini\hdk\LOP_Grid\LOP_Grid.C
```