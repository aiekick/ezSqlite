# Changes nécessaires pour le submodule Grapher

Voici les modifications à apporter au repository Grapher pour supporter les liens personnalisés (comme ERLink avec routing orthogonal).

## Fichier 1: `include/grapher/baseGraph.h`

### Modification 1: Ajouter le typedef CreateLinkFunctor (après ligne 37)

Après la ligne 37: `typedef std::function<void(const BaseGraphWeak&, const BaseNodeWeak&, const BaseSlotWeak&, UserDatas)> SelectSlotAsTargetActionFunctor;`

Ajouter:
```cpp
typedef std::function<BaseLinkPtr(const BaseStyle&, const BaseSlotWeak&, const BaseSlotWeak&)> CreateLinkFunctor;
```

**Contexte complet:**
```cpp
    typedef std::function<void(const BaseGraphWeak&, const BaseNodeWeak&, const BaseSlotWeak&, UserDatas)> SelectSlotAsTargetActionFunctor;
    typedef std::function<BaseLinkPtr(const BaseStyle&, const BaseSlotWeak&, const BaseSlotWeak&)> CreateLinkFunctor;
    typedef ez::Uuid LinkUuid;
```

### Modification 2: Ajouter la variable membre m_CreateLinkFunctor (après ligne 68)

Après la ligne 68: `SelectSlotAsTargetActionFunctor m_SelectSlotAsTargetActionFunctor{nullptr};`

Ajouter:
```cpp
CreateLinkFunctor m_CreateLinkFunctor{nullptr};
```

**Contexte complet:**
```cpp
    SelectSlotActionFunctor m_SelectSlotActionFunctor{nullptr};
    IsNodeSelectedAsTargetActionFunctor m_IsNodeSelectedAsTargetActionFunctor{nullptr};
    SelectSlotAsTargetActionFunctor m_SelectSlotAsTargetActionFunctor{nullptr};
    CreateLinkFunctor m_CreateLinkFunctor{nullptr};
    std::vector<nd::NodeId> m_nodesToCopy;  // for copy/paste
```

### Modification 3: Ajouter les méthodes publiques (après ligne 157)

Après les méthodes `setSelectSlotAsTargetActionFunctor` (ligne 156) et `selectSlotAsTargetAction` (ligne 157)

Ajouter:
```cpp
void setCreateLinkFunctor(const CreateLinkFunctor& vFunctor);
BaseLinkPtr createLinkAction(const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd);
```

**Contexte complet:**
```cpp
    void setSelectSlotAsTargetActionFunctor(const SelectSlotAsTargetActionFunctor& vFunctor);
    void selectSlotAsTargetAction(const BaseGraphWeak& vGraph, const BaseNodeWeak& vNode, const BaseSlotWeak& vSlot, UserDatas vUserDatas);

    void setCreateLinkFunctor(const CreateLinkFunctor& vFunctor);
    BaseLinkPtr createLinkAction(const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd);

    void drawDebugInfos() override;
```

---

## Fichier 2: `src/baseGraph.cpp`

### Modification 1: Implémenter setCreateLinkFunctor et createLinkAction (après ligne 339)

Après la fonction `selectSlotAsTargetAction` (lignes 335-339)

Ajouter:
```cpp
void BaseGraph::setCreateLinkFunctor(const CreateLinkFunctor& vFunctor) {
    m_CreateLinkFunctor = vFunctor;
}

BaseLinkPtr BaseGraph::createLinkAction(const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd) {
    if (m_CreateLinkFunctor != nullptr) {
        return m_CreateLinkFunctor(vParentStyle, vStart, vEnd);
    }
    return BaseLink::create(vParentStyle, vStart, vEnd);
}
```

**Contexte complet:**
```cpp
void BaseGraph::selectSlotAsTargetAction(const BaseGraphWeak& vGraph, const BaseNodeWeak& vNode, const BaseSlotWeak& vSlot, UserDatas vUserDatas) {
    if (m_SelectSlotAsTargetActionFunctor != nullptr) {
        m_SelectSlotAsTargetActionFunctor(vGraph, vNode, vSlot, vUserDatas);
    }
}

void BaseGraph::setCreateLinkFunctor(const CreateLinkFunctor& vFunctor) {
    m_CreateLinkFunctor = vFunctor;
}

BaseLinkPtr BaseGraph::createLinkAction(const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd) {
    if (m_CreateLinkFunctor != nullptr) {
        return m_CreateLinkFunctor(vParentStyle, vStart, vEnd);
    }
    return BaseLink::create(vParentStyle, vStart, vEnd);
}

//////////////////////////////////////////////////////////////////////////////
////// DRAW DEBUG INFOS //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
```

### Modification 2: Modifier m_addVisualLink pour utiliser le functor (ligne 673)

Dans la fonction `BaseGraph::m_addVisualLink` à la ligne 673, remplacer:
```cpp
auto link_ptr = BaseLink::create(m_parentStyle, vStart, vEnd);
```

Par:
```cpp
auto link_ptr = createLinkAction(m_parentStyle, vStart, vEnd);
```

**Contexte complet:**
```cpp
bool BaseGraph::m_addVisualLink(const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd) {
    bool ret = false;
    const auto startPtr = vStart.lock();
    const auto endPtr = vEnd.lock();
    if (startPtr != nullptr && endPtr != nullptr) {
        auto link_ptr = createLinkAction(m_parentStyle, vStart, vEnd);  // <-- CHANGEMENT ICI
        if (link_ptr != nullptr) {
            if (m_links.tryAdd(link_ptr->getUuid(), link_ptr)) {
                if (startPtr->m_links.tryAdd(link_ptr->getUuid(), link_ptr) && endPtr->m_links.tryAdd(link_ptr->getUuid(), link_ptr)) {
                    ret = true;
                } else {
                    EZ_TOOLS_DEBUG_BREAK;
                    LogVarDebugError("Err : fail to add link in both Slots");
                }
            }
        }
    }
    return ret;
}
```

---

## Résumé des changements

Ces modifications permettent à BaseGraph d'accepter un functor personnalisé pour la création de liens. Quand le functor est défini, il est utilisé à la place de `BaseLink::create()`. Cela permet à ezSqlite d'injecter ERLink pour le routing orthogonal sans modifier le code de BaseGraph.

Le pattern suit exactement celui déjà utilisé dans BaseGraph pour les autres fonctors (SelectNodeActionFunctor, etc.), ce qui assure la cohérence architecturale.
