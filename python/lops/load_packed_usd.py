
node = hou.pwd()
geo = node.geometry()

usdfile = "/path/to/usd"

prim = geo.createPacked("PackedUSD")
prim.setIntrinsicValue("usdFileName", usdfile)
prim.setIntrinsicValue("usdPrimPath", '/prim_path')
