"""
技能数据验证器
用于验证技能数据的完整性和正确性
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional


class SkillValidator:
    """技能数据验证器"""

    # 有效的属性类型
    VALID_ATTR_TYPES = {
        "None", "Normal", "Fire", "Water", "Electric", "Grass", "Ice",
        "Fighting", "Poison", "Ground", "Flying", "Cute", "Worm",
        "Rock", "Ghost", "Dragon", "Demon", "Steel", "Light",
        "dFire", "dGrass", "dWater"
    }

    # 有效的技能类型
    VALID_SKILL_TYPES = {
        0: "Physical",
        1: "Magical",
        2: "Status"
    }

    def __init__(self, script_base_dir: Optional[Path] = None):
        """
        初始化验证器

        Args:
            script_base_dir: 脚本文件的基础目录
        """
        self.script_base_dir = script_base_dir or Path(__file__).parent.parent / "assets/skills/scripts"
        self.errors: List[str] = []
        self.warnings: List[str] = []

    def validate_skill(self, skill_data: Dict, row_num: Optional[int] = None) -> bool:
        """
        验证单个技能数据

        Args:
            skill_data: 技能数据字典
            row_num: 行号（用于错误报告）

        Returns:
            bool: 验证是否通过
        """
        # 清空之前的错误和警告
        self.errors.clear()
        self.warnings.clear()

        prefix = f"Row {row_num}: " if row_num else ""
        is_valid = True

        # 验证必需字段
        required_fields = ["id", "name", "skillType", "type"]
        for field in required_fields:
            if field not in skill_data or skill_data[field] is None or skill_data[field] == "":
                self.errors.append(f"{prefix}Missing required field: {field}")
                is_valid = False

        if not is_valid:
            return False

        # 验证ID
        if not self._validate_id(skill_data.get("id"), prefix):
            is_valid = False

        # 验证名称
        if not self._validate_name(skill_data.get("name"), prefix):
            is_valid = False

        # 验证技能类型
        if not self._validate_skill_type(skill_data.get("skillType"), prefix):
            is_valid = False

        # 验证属性类型
        if not self._validate_attr_type(skill_data.get("type"), prefix):
            is_valid = False

        # 验证威力
        if not self._validate_power(skill_data.get("power", 0), prefix):
            is_valid = False

        # 验证PP值
        if not self._validate_pp(skill_data.get("maxPP", 0), prefix):
            is_valid = False

        # 验证优先级
        if not self._validate_priority(skill_data.get("priority", 8), prefix):
            is_valid = False

        # 验证deletable
        if not self._validate_deletable(skill_data.get("deletable", True), prefix):
            is_valid = False

        # 验证脚本路径
        if "scripterPath" in skill_data and skill_data["scripterPath"]:
            # 记录当前错误数量
            error_count_before = len(self.errors)
            path_valid = self._validate_script_path(skill_data["scripterPath"], prefix)
            error_count_after = len(self.errors)

            # 如果产生了新的错误（安全问题），则验证失败
            if error_count_after > error_count_before:
                is_valid = False
            elif not path_valid:
                # 仅文件不存在，添加警告
                self.warnings.append(f"{prefix}Script file not found: {skill_data['scripterPath']}")

        return is_valid

    def _validate_id(self, skill_id, prefix: str) -> bool:
        """验证技能ID"""
        try:
            id_int = int(skill_id)
            if id_int <= 0:
                self.errors.append(f"{prefix}ID must be positive integer, got: {skill_id}")
                return False
            if id_int > 999999:
                self.errors.append(f"{prefix}ID too large (max 999999), got: {skill_id}")
                return False
            return True
        except (ValueError, TypeError):
            self.errors.append(f"{prefix}ID must be integer, got: {skill_id}")
            return False

    def _validate_name(self, name, prefix: str) -> bool:
        """验证技能名称"""
        if not isinstance(name, str):
            self.errors.append(f"{prefix}Name must be string, got: {type(name).__name__}")
            return False

        if not name.strip():
            self.errors.append(f"{prefix}Name cannot be empty")
            return False

        if len(name) > 100:
            self.errors.append(f"{prefix}Name too long (max 100 characters), got: {len(name)}")
            return False

        # 检查是否包含非法字符
        if re.search(r'[<>:"/\\|?*\x00-\x1f]', name):
            self.errors.append(f"{prefix}Name contains invalid characters: {name}")
            return False

        return True

    def _validate_skill_type(self, skill_type, prefix: str) -> bool:
        """验证技能类型"""
        try:
            type_int = int(skill_type)
            if type_int not in self.VALID_SKILL_TYPES:
                self.errors.append(
                    f"{prefix}Invalid skillType: {skill_type}. "
                    f"Must be one of: {list(self.VALID_SKILL_TYPES.keys())}"
                )
                return False
            return True
        except (ValueError, TypeError):
            self.errors.append(f"{prefix}skillType must be integer (0-2), got: {skill_type}")
            return False

    def _validate_attr_type(self, attr_type, prefix: str) -> bool:
        """验证属性类型"""
        if not isinstance(attr_type, str):
            self.errors.append(f"{prefix}Attribute type must be string, got: {type(attr_type).__name__}")
            return False

        if attr_type not in self.VALID_ATTR_TYPES:
            self.errors.append(
                f"{prefix}Invalid attribute type: {attr_type}. "
                f"Must be one of: {sorted(self.VALID_ATTR_TYPES)}"
            )
            return False

        return True

    def _validate_power(self, power, prefix: str) -> bool:
        """验证技能威力"""
        try:
            power_int = int(power)
            if power_int < 0:
                self.errors.append(f"{prefix}Power cannot be negative, got: {power}")
                return False
            if power_int > 999:
                self.warnings.append(f"{prefix}Power is very high ({power}), please verify")
            return True
        except (ValueError, TypeError):
            self.errors.append(f"{prefix}Power must be integer, got: {power}")
            return False

    def _validate_pp(self, max_pp, prefix: str) -> bool:
        """验证PP值"""
        try:
            pp_int = int(max_pp)
            if pp_int < 0:
                self.errors.append(f"{prefix}MaxPP cannot be negative, got: {max_pp}")
                return False
            if pp_int > 99:
                self.warnings.append(f"{prefix}MaxPP is very high ({max_pp}), please verify")
            return True
        except (ValueError, TypeError):
            self.errors.append(f"{prefix}MaxPP must be integer, got: {max_pp}")
            return False

    def _validate_priority(self, priority, prefix: str) -> bool:
        """验证优先级"""
        try:
            priority_int = int(priority)
            if priority_int < 0 or priority_int > 16:
                self.errors.append(f"{prefix}Priority must be between 0-16, got: {priority}")
                return False
            return True
        except (ValueError, TypeError):
            self.errors.append(f"{prefix}Priority must be integer, got: {priority}")
            return False

    def _validate_deletable(self, deletable, prefix: str) -> bool:
        """验证是否可删除标志"""
        if not isinstance(deletable, bool):
            # 尝试转换字符串
            if isinstance(deletable, str):
                lower_val = deletable.lower()
                if lower_val in ("true", "1", "yes"):
                    return True
                elif lower_val in ("false", "0", "no"):
                    return True
            self.errors.append(f"{prefix}Deletable must be boolean, got: {deletable}")
            return False
        return True

    def _validate_script_path(self, script_path: str, prefix: str) -> bool:
        """验证脚本路径"""
        if not isinstance(script_path, str) or not script_path.strip():
            return True  # 空路径是允许的

        # 检查路径中的非法字符
        if re.search(r'[<>:"|?*\x00-\x1f]', script_path):
            self.errors.append(f"{prefix}Script path contains invalid characters: {script_path}")
            return False

        # 防止路径遍历攻击
        if '..' in script_path or script_path.startswith('/') or script_path.startswith('\\'):
            self.errors.append(f"{prefix}Invalid path (potential traversal): {script_path}")
            return False

        # 检查是否为Lua文件
        if not script_path.endswith('.lua'):
            self.warnings.append(f"{prefix}Script path should end with .lua: {script_path}")
            # 对于非.lua文件，如果其他检查通过，只警告不阻止
            # 但继续检查文件存在性

        # 检查文件是否存在
        full_path = self.script_base_dir / script_path

        # 确保路径在允许的目录内（防止符号链接攻击）
        try:
            resolved_path = full_path.resolve()
            resolved_base = self.script_base_dir.resolve()
            # 检查解析后的路径是否在基础目录内
            try:
                resolved_path.relative_to(resolved_base)
            except ValueError:
                self.errors.append(f"{prefix}Path outside allowed directory: {script_path}")
                return False
        except (OSError, RuntimeError) as e:
            self.errors.append(f"{prefix}Error resolving path: {script_path} - {str(e)}")
            return False

        # 文件不存在只是警告，不是错误（因为可能稍后创建）
        if not full_path.exists():
            return False  # 返回False表示文件不存在，但不添加错误

        if not full_path.is_file():
            self.errors.append(f"{prefix}Script path is not a file: {script_path}")
            return False

        return True

    def validate_batch(self, skills_data: List[Dict]) -> Tuple[bool, List[str], List[str]]:
        """
        批量验证技能数据

        Args:
            skills_data: 技能数据列表

        Returns:
            Tuple[bool, List[str], List[str]]: (是否全部通过, 错误列表, 警告列表)
        """
        all_errors = []
        all_warnings = []
        all_valid = True

        # 检查ID重复
        id_set = set()
        for idx, skill in enumerate(skills_data, 1):
            skill_id = skill.get("id")
            if skill_id in id_set:
                all_errors.append(f"Duplicate ID found: {skill_id} at row {idx}")
                all_valid = False
            else:
                id_set.add(skill_id)

            # 验证单个技能
            if not self.validate_skill(skill, idx):
                all_valid = False

            all_errors.extend(self.errors)
            all_warnings.extend(self.warnings)

        return all_valid, all_errors, all_warnings

    def get_errors(self) -> List[str]:
        """获取错误列表"""
        return self.errors.copy()

    def get_warnings(self) -> List[str]:
        """获取警告列表"""
        return self.warnings.copy()
