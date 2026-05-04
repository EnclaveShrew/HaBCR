#pragma once

namespace RE
{
class hkbNode;

struct hkbNodeInfo
{
    std::uint64_t unk00;
    hkbNode *nodeClone;
};
static_assert(offsetof(hkbNodeInfo, nodeClone) == 0x08);
}
