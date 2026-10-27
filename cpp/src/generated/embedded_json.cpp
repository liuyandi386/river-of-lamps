// 由 cpp/tools/make_embedded_assets.py 生成，请勿手改。
#include "generated/embedded_assets.h"

namespace river::embedded {

const char* const kIndexJson = R"RIVERJSON({
  "levels": [
    { "id": "level_01", "name": "细流" },
    { "id": "level_02", "name": "倒影" },
    { "id": "level_03", "name": "第一盏灯" },
    { "id": "level_04", "name": "汇流" },
    { "id": "level_05", "name": "分岔" }
  ]
}
)RIVERJSON";
const char* const kElementsJson = R"RIVERJSON({
  "schema": 1,
  "elements": [
    {
      "id": "source",
      "batch": 1,
      "name": { "zh": "信号源" },
      "footprint": [[0, 0]],
      "ports": { "in": [], "out": ["E"] },
      "delay": 0,
      "params": { "mode": ["high", "low", "pulse"], "pulse_at_tick": 0 },
      "rule": "source_rule"
    },
    {
      "id": "wire",
      "batch": 1,
      "name": { "zh": "灯槽" },
      "footprint": [[0, 0]],
      "ports": { "in": ["flow"], "out": ["flow"] },
      "delay": 1,
      "params": {},
      "rule": "wire_rule"
    },
    {
      "id": "and",
      "batch": 1,
      "name": { "zh": "与门" },
      "footprint": [[0, 0]],
      "ports": { "in": ["W", "S"], "out": ["E"] },
      "delay": 0,
      "params": {},
      "rule": "gate_rule"
    },
    {
      "id": "or",
      "batch": 1,
      "name": { "zh": "或门" },
      "footprint": [[0, 0]],
      "ports": { "in": ["W", "S"], "out": ["E"] },
      "delay": 0,
      "params": {},
      "rule": "gate_rule"
    },
    {
      "id": "not",
      "batch": 1,
      "name": { "zh": "非门" },
      "footprint": [[0, 0]],
      "ports": { "in": ["W"], "out": ["E"] },
      "delay": 0,
      "params": {},
      "rule": "gate_rule"
    },
    {
      "id": "target",
      "batch": 1,
      "name": { "zh": "目标端口" },
      "footprint": [[0, 0]],
      "ports": { "in": ["W"], "out": [] },
      "delay": 0,
      "params": { "expect": [], "start_tick": 0 },
      "rule": "target_rule"
    }
  ]
}
)RIVERJSON";

const char* const kLevel01Json = R"RIVERJSON({
  "schema": 1,
  "id": "level_01",
  "name": { "zh": "细流" },
  "chapter": "tutorial",
  "grid": { "w": 8, "h": 8 },
  "tick_budget": 24,
  "max_components": 10,
  "teach": {
    "zh": "灯槽每格延迟 1 拍。源会一直亮；从源开始铺灯槽，铺够 6 格，目标才会在第 6 拍看到亮。"
  },
  "preset": [
    { "type": "source", "pos": [0, 2], "params": { "mode": "high" } }
  ],
  "available": ["wire"],
  "targets": [
    { "id": "t1", "pos": [7, 2], "expect": [1], "start_tick": 6 }
  ],
  "interference": [],
  "hint": {
    "zh": "从信号源按住左键，沿网格拖出灯槽。灯槽每格延迟 1 拍，数一数信号要走几拍。"
  },
  "reference_finish": 6,
  "star_thresholds": { "three": 6, "two": 8, "one": 12 },
  "reference_solution": {
    "wires": [
      [[1, 2], [2, 2], [3, 2], [4, 2], [5, 2], [6, 2], [7, 2]]
    ]
  }
}
)RIVERJSON";
const char* const kLevel02Json = R"RIVERJSON({
  "schema": 1,
  "id": "level_02",
  "name": { "zh": "倒影" },
  "chapter": "tutorial",
  "grid": { "w": 8, "h": 8 },
  "tick_budget": 24,
  "max_components": 10,
  "teach": {
    "zh": "非门会把高变成低、低变成高。源只亮 1 拍；把它送进非门后，目标会先看到灭，下一拍才亮。先把源接到非门左侧。"
  },
  "preset": [
    { "type": "source", "pos": [0, 2], "params": { "mode": "pulse", "pulse_at_tick": 0 } },
    { "type": "not", "pos": [3, 2], "params": {} }
  ],
  "available": ["wire"],
  "targets": [
    { "id": "t1", "pos": [4, 2], "expect": [0, 1], "start_tick": 2 }
  ],
  "interference": [],
  "hint": {
    "zh": "从源向右铺两格到非门的左侧。第 2 拍非门会灭，第 3 拍再亮；绕远路会让它晚 2 拍。"
  },
  "reference_finish": 3,
  "star_thresholds": { "three": 3, "two": 5, "one": 9 },
  "reference_solution": {
    "wires": [
      [[1, 2], [2, 2], [3, 2]]
    ]
  }
}
)RIVERJSON";
const char* const kLevel03Json = R"RIVERJSON({
  "schema": 1,
  "id": "level_03",
  "name": { "zh": "第一盏灯" },
  "chapter": "tutorial",
  "grid": { "w": 8, "h": 8 },
  "tick_budget": 24,
  "max_components": 10,
  "preset": [
    { "type": "source", "pos": [0, 3], "params": { "mode": "pulse", "pulse_at_tick": 0 } },
    { "type": "not", "pos": [4, 3], "params": {} }
  ],
  "available": ["wire"],
  "targets": [
    { "id": "t1", "pos": [5, 3], "expect": [0, 1], "start_tick": 3 }
  ],
  "interference": [],
  "hint": {
    "zh": "非门会把高变低、低变高。脉冲经过非门时，目标会先看到“灭”，下一拍才看到“亮”。"
  },
  "reference_finish": 4,
  "star_thresholds": { "three": 4, "two": 6, "one": 9 },
  "reference_solution": {
    "wires": [
      [[1, 3], [2, 3], [3, 3], [4, 3]]
    ]
  }
}
)RIVERJSON";
const char* const kLevel04Json = R"RIVERJSON({
  "schema": 1,
  "id": "level_04",
  "name": { "zh": "汇流" },
  "chapter": "combo",
  "grid": { "w": 8, "h": 8 },
  "tick_budget": 24,
  "max_components": 10,
  "preset": [
    { "type": "source", "pos": [0, 4], "params": { "mode": "pulse", "pulse_at_tick": 0 } },
    { "type": "source", "pos": [5, 7], "params": { "mode": "pulse", "pulse_at_tick": 0 } },
    { "type": "and", "pos": [5, 4], "params": {} }
  ],
  "available": ["wire"],
  "targets": [
    { "id": "t1", "pos": [6, 4], "expect": [1], "start_tick": 4 }
  ],
  "interference": [],
  "hint": {
    "zh": "与门要两个输入同时为高才亮。两条灯槽都要 4 拍，让两个脉冲同一拍到达。"
  },
  "reference_finish": 4,
  "star_thresholds": { "three": 4, "two": 6, "one": 9 },
  "reference_solution": {
    "wires": [
      [[1, 4], [2, 4], [3, 4], [4, 4], [5, 4]],
      [[6, 7], [6, 6], [6, 5], [5, 5], [5, 4]]
    ]
  }
}
)RIVERJSON";
const char* const kLevel05Json = R"RIVERJSON({
  "schema": 1,
  "id": "level_05",
  "name": { "zh": "分岔" },
  "chapter": "combo",
  "grid": { "w": 8, "h": 8 },
  "tick_budget": 24,
  "max_components": 10,
  "preset": [
    { "type": "source", "pos": [0, 4], "params": { "mode": "pulse", "pulse_at_tick": 0 } },
    { "type": "source", "pos": [5, 7], "params": { "mode": "pulse", "pulse_at_tick": 0 } },
    { "type": "or", "pos": [5, 4], "params": {} }
  ],
  "available": ["wire"],
  "targets": [
    { "id": "t1", "pos": [6, 4], "expect": [1, 0, 1], "start_tick": 4 }
  ],
  "interference": [],
  "hint": {
    "zh": "或门只要有一路亮就亮。让左边那路 4 拍到，下面那路 6 拍到：目标会看到亮、灭、亮。"
  },
  "reference_finish": 6,
  "star_thresholds": { "three": 6, "two": 8, "one": 12 },
  "reference_solution": {
    "wires": [
      [[1, 4], [2, 4], [3, 4], [4, 4], [5, 4]],
      [[6, 7], [7, 7], [7, 6], [7, 5], [6, 5], [5, 5], [5, 4]]
    ]
  }
}
)RIVERJSON";

const LevelBlob kLevels[] = {
    {"level_01", kLevel01Json},
    {"level_02", kLevel02Json},
    {"level_03", kLevel03Json},
    {"level_04", kLevel04Json},
    {"level_05", kLevel05Json},
};
const std::size_t kLevelCount = sizeof(kLevels) / sizeof(kLevels[0]);

}  // namespace river::embedded
