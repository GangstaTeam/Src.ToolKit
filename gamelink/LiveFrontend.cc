#include "LiveFrontend.hh"

#include <SDK/.SDK.hh>

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace tk::link;
namespace frontend = pure3d::frontend;

namespace
{

frontend::Child* ChildOf(uint32_t id)
{
    return reinterpret_cast<frontend::Child*>(static_cast<uintptr_t>(id));
}

uint32_t IdOf(const frontend::Child* child)
{
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(child));
}

bool Submit(frontend::Child* child)
{
    __try
    {
        child->Draw();
        return true;
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool Bounds(frontend::Drawable* drawable, int32_t out[4])
{
    __try
    {
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        drawable->GetBoundingBox(x0, y0, x1, y1);
        out[0] = x0, out[1] = y0, out[2] = x1, out[3] = y1;
        return true;
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

NodeType TypeOf(const frontend::Child* child)
{
    if (child->IsA<frontend::Screen>())
    {
        return NodeType::Screen;
    }

    if (child->IsA<frontend::Page>())
    {
        return NodeType::Page;
    }

    if (child->IsA<frontend::Layer>())
    {
        return NodeType::Layer;
    }

    if (child->IsA<frontend::Group>())
    {
        return NodeType::Group;
    }

    if (child->IsA<frontend::Sprite>())
    {
        return NodeType::Sprite;
    }

    if (child->IsA<frontend::Text>())
    {
        return NodeType::Text;
    }

    if (child->IsA<frontend::Pure3dObject>())
    {
        return NodeType::Pure3dObject;
    }

    if (child->IsA<frontend::Polygon>())
    {
        return NodeType::Polygon;
    }

    if (child->IsA<frontend::RoundedRectangle>())
    {
        return NodeType::RoundedRectangle;
    }

    return NodeType::Unknown;
}

bool IsParent(NodeType type)
{
    return type == NodeType::Screen || type == NodeType::Page || type == NodeType::Layer || type == NodeType::Group;
}

bool IsDrawable(NodeType type)
{
    return type >= NodeType::Sprite && type <= NodeType::RoundedRectangle;
}

float AsFloat(uint32_t bits)
{
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

}

bool LiveFrontend::Walk()
{
    m_nodes.clear();
    m_index.clear();
    __try
    {
        WalkProjects();
        return true;
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        m_nodes.clear();
        m_index.clear();
        return false;
    }
}

void LiveFrontend::WalkProjects()
{
    frontend::App* app = frontend::App::GetInstance();

    if (!app)
    {
        return;
    }

    for (int slot = 0; slot < frontend::App::MaxProjects; ++slot)
    {
        frontend::Project* project = app->m_pProjects[slot];

        if (!project)
        {
            continue;
        }

        NodeRecord node = {};
        node.id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(project));
        node.key = project->m_Name;
        node.type = static_cast<uint8_t>(NodeType::Project);
        node.project = static_cast<uint8_t>(slot);
        node.flags = kNodeVisible | kNodeShown;
        node.alpha = 255;
        m_index[node.id] = m_nodes.size();
        m_nodes.push_back(node);

        for (uint32_t i = 0; frontend::Screen* screen = project->GetScreen(i); ++i)
        {
            WalkGuarded(screen, node.id, node.project, true, 0);
        }
    }
}

void LiveFrontend::WalkGuarded(frontend::Child* child, uint32_t parent, uint8_t project, bool shown, int depth)
{
    __try
    {
        WalkObject(child, parent, project, shown, depth);
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void LiveFrontend::WalkObject(frontend::Child* child, uint32_t parent, uint8_t project, bool shown, int depth)
{
    uint32_t id = IdOf(child);

    if (!child || depth > kMaxDepth || m_nodes.size() >= kMaxNodes || m_index.count(id))
    {
        return;
    }

    NodeType type = TypeOf(child);
    NodeRecord node = {};
    node.id = id;
    node.parent = parent;
    node.key = child->m_Name;
    node.type = static_cast<uint8_t>(type);
    node.project = project;
    node.alpha = m_pulse.id == id ? m_pulse.alpha : child->m_Alpha;
    bool visible = true;

    if (IsParent(type))
    {
        visible = child->m_bVisible;
    }
    else if (IsDrawable(type))
    {
        auto* drawable = static_cast<frontend::Drawable*>(child);
        visible = drawable->m_bShow;
        node.justification = static_cast<uint8_t>(drawable->m_nJustificationX | (drawable->m_nJustificationY << 3));
        node.colour = drawable->mColour.c;
        node.x = drawable->mPosX;
        node.y = drawable->mPosY;
        node.offsetX = drawable->m_fOffsetX;
        node.offsetY = drawable->m_fOffsetY;
        node.width = drawable->mWidth;
        node.height = drawable->mHeight;

        if (Bounds(drawable, node.bounds))
        {
            int dx = static_cast<int>(node.offsetX), dy = static_cast<int>(node.offsetY);
            node.bounds[0] += dx, node.bounds[2] += dx, node.bounds[1] += dy, node.bounds[3] += dy;
            node.flags |= kNodeHasBounds;
        }
    }

    shown = shown && visible;
    node.flags |= (visible ? kNodeVisible : 0) | (shown ? kNodeShown : 0) | (m_originals.count(id) ? kNodeEdited : 0);
    m_index[id] = m_nodes.size();
    m_nodes.push_back(node);

    if (!IsParent(type))
    {
        return;
    }

    auto* children = static_cast<frontend::Parent*>(child);

    if (!children->m_ChildrenBegin || children->m_ChildrenEnd < children->m_ChildrenBegin)
    {
        return;
    }

    uint32_t count = std::min<uint32_t>(children->GetNumChildren(), kMaxChildren);

    for (uint32_t i = 0; i < count; ++i)
    {
        WalkGuarded(children->GetChild(i), id, project, shown, depth + 1);
    }
}

const NodeRecord* LiveFrontend::Find(uint32_t id) const
{
    auto found = m_index.find(id);
    return found == m_index.end() ? nullptr : &m_nodes[found->second];
}

void LiveFrontend::Remember(const NodeRecord& node, Field field)
{
    Original& original = m_originals[node.id];

    if (original.mask && original.key != node.key)
    {
        original = {};
    }

    uint8_t bit = static_cast<uint8_t>(1u << static_cast<int>(field));

    if (original.mask & bit)
    {
        return;
    }

    original.key = node.key;
    original.mask |= bit;

    switch (field)
    {
    case Field::Visible:
        original.visible = (node.flags & kNodeVisible) != 0;
        break;
    case Field::Alpha:
        original.alpha = node.alpha;
        break;
    case Field::X:
        original.x = node.x;
        break;
    case Field::Y:
        original.y = node.y;
        break;
    case Field::Colour:
        original.colour = node.colour;
        break;
    }
}

void LiveFrontend::Write(const NodeRecord& node, Field field, uint32_t value)
{
    NodeType type = static_cast<NodeType>(node.type);

    if (type == NodeType::Project || type == NodeType::Unknown)
    {
        return;
    }

    frontend::Child* child = ChildOf(node.id);
    auto* drawable = static_cast<frontend::Drawable*>(child);
    __try
    {
        switch (field)
        {
        case Field::Visible:
            if (IsParent(type))
            {
                child->m_bVisible = value != 0;
            }
            else
            {
                drawable->m_bShow = value ? 1 : 0;
            }
            break;
        case Field::Alpha:
            if (m_pulse.id == node.id)
            {
                m_pulse.alpha = static_cast<uint8_t>(value);
            }
            else
            {
                child->m_Alpha = static_cast<uint8_t>(value);
            }
            break;
        case Field::X:
            if (IsDrawable(type))
            {
                drawable->mPosX = AsFloat(value);
            }
            break;
        case Field::Y:
            if (IsDrawable(type))
            {
                drawable->mPosY = AsFloat(value);
            }
            break;
        case Field::Colour:
            if (IsDrawable(type) && type != NodeType::Polygon)
            {
                drawable->mColour.c = value;
            }
            break;
        }
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }

    Submit(child);
}

void LiveFrontend::SetField(uint32_t id, Field field, uint32_t value)
{
    const NodeRecord* node = Find(id);

    if (!node)
    {
        return;
    }

    Remember(*node, field);
    Write(*node, field, value);
}

void LiveFrontend::Revert(uint32_t id)
{
    for (auto it = m_originals.begin(); it != m_originals.end();)
    {
        if (id && it->first != id)
        {
            ++it;
            continue;
        }

        const NodeRecord* node = Find(it->first);
        const Original& original = it->second;

        if (node && node->key == original.key)
        {
            uint32_t x, y;
            std::memcpy(&x, &original.x, 4);
            std::memcpy(&y, &original.y, 4);

            if (original.mask & 1)
            {
                Write(*node, Field::Visible, original.visible);
            }

            if (original.mask & 2)
            {
                Write(*node, Field::Alpha, original.alpha);
            }

            if (original.mask & 4)
            {
                Write(*node, Field::X, x);
            }

            if (original.mask & 8)
            {
                Write(*node, Field::Y, y);
            }

            if (original.mask & 16)
            {
                Write(*node, Field::Colour, original.colour);
            }
        }

        it = m_originals.erase(it);
    }
}

void LiveFrontend::ClearHighlight()
{
    if (!m_pulse.id)
    {
        return;
    }

    const NodeRecord* node = Find(m_pulse.id);

    if (node && node->key == m_pulse.key)
    {
        frontend::Child* child = ChildOf(m_pulse.id);
        __try
        {
            child->m_Alpha = m_pulse.alpha;
        }

        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        Submit(child);
    }

    m_pulse = {};
}

void LiveFrontend::UpdateHighlight(double seconds)
{
    const NodeRecord* node = m_highlight && m_selected ? Find(m_selected) : nullptr;

    if (node &&
        (node->type == static_cast<uint8_t>(NodeType::Project) ||
            node->type == static_cast<uint8_t>(NodeType::Unknown)))
    {
        node = nullptr;
    }

    if (m_pulse.id && (!node || node->id != m_pulse.id))
    {
        ClearHighlight();
    }

    if (!node)
    {
        return;
    }

    if (!m_pulse.id)
    {
        m_pulse = {node->id, node->key, node->alpha};
    }

    double wave = 0.5 + 0.5 * std::sin(seconds * 9.0);
    frontend::Child* child = ChildOf(node->id);
    __try
    {
        child->m_Alpha = static_cast<uint8_t>(kPulseLow + (255 - kPulseLow) * wave);
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        m_pulse = {};
        return;
    }

    Submit(child);
}
