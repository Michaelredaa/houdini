# -----------------------------------------------------------------------------
# © 2024 eng.michaelreda@gmail.com. All rights reserved.
# 
# -----------------------------------------------------------------------------


"""
This script simplifies the process of creating a debug shader for MaterialX networks in Houdini. 
It connects a debug shader to the selected node, allowing you to easily inspect and debug the network.

By default, the script connects the output directly to the shader. If the selected node has multiple outputs 
(like R, G, B channels), you can toggle between them by repeatedly using the shortcut.

How to Use:
1- Add the Script to a Shelf Button:
    * Right-click on an existing shelf and select New Tool.
    * In the tool settings, go to the Script tab and paste the script.
    
2- Assign a Hotkey:
    * Go to the Hotkeys tab within the tool settings.
    * Assign a convenient hotkey to the button for quick access.
    
3- Use the Script in MaterialX Subnet:
    * When inside a MaterialX subnet, press the assigned hotkey. 
    
The script will automatically create the debug shader and make the necessary connections.
This tool enhances your workflow by providing a quick and easy way to debug MaterialX networks directly within Houdini.

"""


import hou


# Debug Node
debug_node_type = "mtlxsurface_unlit"
debug_node_name = "DEBUG"
debug_input_name = "emission_color"

# Output Node
output_node_name = "surface_output"
debug_output_name = "out"


def create_connect_debug():
    """
    Create a debug node, position it relative to the output node, and connect it
    to the selected node's output. If no node is selected, the user is prompted.
    """

    editors = [pane for pane in hou.ui.paneTabs() if isinstance(pane, hou.NetworkEditor) and pane.isUnderCursor()]

    if not editors:
        print("Please open and place your cursor on the correct network editor pane.")
        return

    parent = editors[-1].pwd()

    out_node = parent.node(output_node_name)

    if out_node is None:
        print(f"Output node '{output_node_name}' not found in the subnet, please use this inside materialx subnet.")
        return

    debug_node = parent.node(debug_node_name)
    if debug_node is None:
        debug_node = parent.createNode(debug_node_type, debug_node_name)
        pos = out_node.position()
        pos[0] += 10
        pos[1] += 10
        debug_node.setPosition(pos)

    out_node.setInput(0, debug_node)

    selected = hou.selectedNodes()
    if selected:
        view_node = selected[0]
        try:
            debug_node.setNamedInput(debug_input_name, view_node, debug_output_name)
        except hou.InvalidInput:

            connections = view_node.outputConnectors()
            idx = 0
            for i, conn in enumerate(connections):
                if conn:
                    if [x for x in conn if x.outputItem().name() == debug_node_name]:
                        idx = i
            debug_node.setNamedInput(debug_input_name, view_node, (idx + 1) % len(connections))


create_connect_debug()
