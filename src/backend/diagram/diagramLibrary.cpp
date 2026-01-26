#include <backend/diagram/diagramLibrary.h>

// Internal Nodes
#include <backend/diagram/nodes/TableNode.h>

BaseLibrary DiagramLibrary::getLibrary() {
    BaseLibrary lib;
    ////////////////////////////////////////

    // INPUTS
    lib.addLibraryEntry(BaseLibrary::LibraryEntry(
        "Diagram",
        "Table",
        "DIAGRAM_TABLE_NODE",  //
        {},
        {"FIELD", "FIELD"},
        [](const BaseGraphWeak& vGraph) { return vGraph.lock()->createChildNode<TableNode>(); }));

    ////////////////////////////////////////
    return lib;
}
