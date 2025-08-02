@echo off
del interaction.vfp
cgc -quiet -q -profile arbvp1 cgvp_interaction.cg >> interaction.vfp
echo #====================================================================== >> interaction.vfp
cgc -quiet -q -profile arbfp1 cgfp_interaction.cg >> interaction.vfp
