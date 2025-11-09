# Rocoarena Tools 技能数据管理工具

**environment: rocoarena**

```bash
conda activate rocoarena
```

## 工具概览

本工具集用于管理和处理技能数据，包括数据导入、验证、差异比对和hash计算。

### 工具列表

1. **skill_importer.py** - 技能数据导入工具
2. **skill_validator.py** - 技能数据验证器
3. **diff_exporter.py** - 差异比对和导出工具
4. **hash_utils.py** - Hash计算工具

---

## 1. skill_importer.py - 技能数据导入工具

从Excel或CSV文件批量导入技能数据到JSON格式。

### 功能特性

- 支持Excel (.xlsx, .xls) 和 CSV 文件
- 自动类型转换和数据验证
- 灵活的列名映射（支持中英文列名）
- 自动生成文件hash值
- 详细的错误和警告报告

### 使用方法

#### 基本用法

```bash
# 从Excel文件导入
python skill_importer.py skills_data.xlsx

# 从CSV文件导入
python skill_importer.py skills_data.csv

# 指定Excel工作表
python skill_importer.py skills_data.xlsx --sheet "Sheet2"

# 指定CSV编码
python skill_importer.py skills_data.csv --encoding gbk

# 指定输出目录
python skill_importer.py skills_data.xlsx --output-dir /path/to/output
```

#### 创建模板

```bash
# 创建CSV模板文件
python skill_importer.py --create-template template.csv
```

#### 跳过验证（不推荐）

```bash
python skill_importer.py skills_data.xlsx --no-validate
```

### 数据格式

#### 必需字段

| 字段名 | 类型 | 说明 | 示例 |
|--------|------|------|------|
| id | int | 技能ID (唯一) | 1 |
| name | string | 技能名称 | "火焰拳" |
| skillType | int | 技能类型 (0-物理, 1-魔法, 2-状态) | 0 |
| type | string | 属性类型 | "Fire" |

#### 可选字段

| 字段名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| description | string | "" | 技能描述 |
| power | int | 0 | 技能威力 |
| maxPP | int | 0 | 最大PP值 |
| deletable | bool | true | 是否可删除 |
| priority | int | 8 | 优先级 (0-16) |
| scripterPath | string | "" | Lua脚本路径 |

#### 有效的属性类型（type字段）

```
None, Normal, Fire, Water, Electric, Grass, Ice, Fighting, Poison,
Ground, Flying, Cute, Worm, Rock, Ghost, Dragon, Demon, Steel,
Light, dFire, dGrass, dWater
```

### 示例数据

```csv
id,name,description,skillType,type,power,maxPP,deletable,priority,scripterPath
1,火焰拳,使用火焰包裹的拳头攻击对手,0,Fire,75,15,true,8,skills/fire_punch.lua
2,水炮,向对手喷射强力水流,1,Water,110,5,true,8,skills/hydro_pump.lua
3,催眠术,使对手陷入睡眠状态,2,Cute,0,20,true,8,skills/hypnosis.lua
```

---

## 2. skill_validator.py - 技能数据验证器

验证技能数据的完整性和正确性。

### 验证规则

#### 数据类型验证
- ID: 必须为正整数，范围 1-999999
- 名称: 不能为空，最大100字符，不包含非法字符
- 技能类型: 必须为 0、1 或 2
- 属性类型: 必须在有效列表中
- 威力: 非负整数，>999 时警告
- PP值: 非负整数，>99 时警告
- 优先级: 0-16之间的整数
- 可删除标志: 布尔值

#### 路径验证
- 脚本路径: 检查文件是否存在，是否为.lua文件

#### 批量验证
- ID重复检查
- 批量错误收集

### 编程接口

```python
from skill_validator import SkillValidator

# 创建验证器
validator = SkillValidator()

# 验证单个技能
skill_data = {
    "id": 1,
    "name": "测试技能",
    "skillType": 0,
    "type": "Normal"
}

is_valid = validator.validate_skill(skill_data)
if not is_valid:
    print("Errors:", validator.get_errors())
    print("Warnings:", validator.get_warnings())

# 批量验证
skills_list = [skill1, skill2, skill3]
is_valid, errors, warnings = validator.validate_batch(skills_list)
```

---

## 3. diff_exporter.py - 差异比对工具

比较两个版本的技能数据，生成差异报告。

### 功能特性

- 检测新增、删除和修改的技能
- 详细的字段级差异对比
- Hash值变化检测
- JSON格式报告输出

### 使用方法

```bash
# 比较两个目录
python diff_exporter.py /path/to/old_skills --new-dir /path/to/new_skills

# 输出到JSON文件
python diff_exporter.py /path/to/old_skills --output report.json

# 静默模式（仅生成文件）
python diff_exporter.py /path/to/old_skills --output report.json --quiet
```

### 报告格式

```json
{
  "timestamp": "2024-11-10T12:00:00",
  "summary": {
    "added": 5,
    "modified": 12,
    "deleted": 2,
    "unchanged": 100
  },
  "details": {
    "added": [...],
    "modified": [
      {
        "id": 1,
        "name": "火焰拳",
        "changes": {
          "power": {"old": 70, "new": 75},
          "maxPP": {"old": 10, "new": 15}
        }
      }
    ],
    "deleted": [...]
  }
}
```

---

## 4. hash_utils.py - Hash计算工具

计算技能数据的MD5 hash值，用于版本控制和变更检测。

### 功能

- **stable_jsonDumps()**: 固定顺序JSON序列化
- **md5_hash()**: 计算字符串的MD5
- **hash_skillData()**: 计算技能数据hash（包含JSON和脚本内容）

### 使用示例

```python
from hash_utils import hash_skillData, stable_jsonDumps

skill_data = {"id": 1, "name": "测试技能", ...}
script_content = "-- Lua script content"

hash_value = hash_skillData(skill_data, script_content)
print(f"Hash: {hash_value}")
```

---

## 安全注意事项

### 已实现的安全措施

1. **路径安全**
   - 检查路径非法字符
   - 防止路径遍历攻击
   - 验证文件扩展名

2. **输入验证**
   - 严格的数据类型检查
   - 数值范围限制
   - 字符串长度限制
   - 特殊字符过滤

3. **错误处理**
   - 捕获并处理所有异常
   - 详细的错误报告
   - 安全的文件操作

4. **编码处理**
   - 支持多种编码格式
   - UTF-8优先
   - 自动编码检测

### 最佳实践

1. **总是启用验证**
   - 不要使用 `--no-validate` 选项
   - 验证可以捕获大部分错误

2. **备份数据**
   - 导入前备份现有数据
   - 使用版本控制系统

3. **增量导入**
   - 分批导入大量数据
   - 及时检查错误和警告

4. **脚本路径**
   - 使用相对路径
   - 确保脚本文件存在

---

## 依赖项

### Python版本
- Python 3.7+

### 必需库
```bash
# 基础功能
pip install pandas openpyxl

# 可选（用于xlsx支持）
pip install xlrd
```

### 安装依赖

```bash
# 创建conda环境
conda create -n rocoarena python=3.10
conda activate rocoarena

# 安装依赖
pip install pandas openpyxl xlrd
```

---

## 常见问题

### 1. 导入失败：编码错误

```bash
# 尝试指定编码
python skill_importer.py data.csv --encoding gbk
python skill_importer.py data.csv --encoding utf-8-sig
```

### 2. 脚本路径验证失败

确保脚本文件存在于 `arena/assets/skills/scripts/` 目录下。

### 3. ID重复错误

检查数据源，确保每个技能ID唯一。

### 4. 属性类型无效

确保使用正确的属性类型名称（区分大小写）。

---

## 工作流程示例

### 完整的数据导入流程

```bash
# 1. 创建CSV模板
python skill_importer.py --create-template my_skills.csv

# 2. 编辑CSV文件，填入技能数据

# 3. 验证并导入
python skill_importer.py my_skills.csv

# 4. 比对变更（如果有旧版本）
python diff_exporter.py /backup/old_skills --output changes.json

# 5. 检查报告
cat changes.json
```

---

## 故障排除

### 检查工具版本

```bash
python --version
pip list | grep pandas
```

### 测试环境

```bash
# 测试导入工具
python -c "from skill_importer import SkillImporter; print('OK')"

# 测试验证器
python -c "from skill_validator import SkillValidator; print('OK')"
```

### 日志和调试

所有工具都会输出详细的错误信息到标准输出，包括：
- 行号
- 字段名
- 错误原因
- 建议的修复方法

---

## 更新日志

### v1.0.0 (2024-11-10)
- 初始版本
- 实现Excel/CSV导入功能
- 实现数据验证器
- 实现差异比对工具
- 完善hash计算功能
- 添加详细文档