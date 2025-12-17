#!/usr/bin/env python3
"""
技能数据管理工具测试程序
测试导入、验证、差异比对等功能
"""

import os
import sys
import json
import shutil
from pathlib import Path
from datetime import datetime

# 添加当前目录到Python路径
sys.path.insert(0, str(Path(__file__).parent))

from skill_importer import SkillImporter
from skill_validator import SkillValidator
from diff_exporter import SkillDiffExporter


class TestRunner:
    """测试运行器"""

    def __init__(self):
        self.test_dir = Path(__file__).parent / "test_output"
        self.test_dir.mkdir(exist_ok=True)

        self.passed = 0
        self.failed = 0
        self.tests_run = []

    def log(self, message, level="INFO"):
        """打印日志"""
        colors = {
            "INFO": "\033[94m",    # 蓝色
            "PASS": "\033[92m",    # 绿色
            "FAIL": "\033[91m",    # 红色
            "WARN": "\033[93m",    # 黄色
            "END": "\033[0m"       # 重置
        }

        color = colors.get(level, colors["INFO"])
        print(f"{color}[{level}]{colors['END']} {message}")

    def assert_true(self, condition, test_name, message=""):
        """断言条件为真"""
        if condition:
            self.passed += 1
            self.log(f"✓ {test_name} {message}", "PASS")
            self.tests_run.append((test_name, True))
            return True
        else:
            self.failed += 1
            self.log(f"✗ {test_name} {message}", "FAIL")
            self.tests_run.append((test_name, False))
            return False

    def assert_false(self, condition, test_name, message=""):
        """断言条件为假"""
        return self.assert_true(not condition, test_name, message)

    def print_separator(self, title=""):
        """打印分隔线"""
        width = 70
        if title:
            padding = (width - len(title) - 2) // 2
            print("\n" + "=" * padding + f" {title} " + "=" * padding)
        else:
            print("\n" + "=" * width)

    def cleanup(self):
        """清理测试文件"""
        if self.test_dir.exists():
            shutil.rmtree(self.test_dir)
        self.test_dir.mkdir(exist_ok=True)

    # ==================== 验证器测试 ====================

    def test_validator_valid_skill(self):
        """测试验证器 - 有效技能"""
        self.print_separator("测试验证器 - 有效技能")

        validator = SkillValidator()
        valid_skill = {
            "id": 1,
            "name": "测试技能",
            "description": "这是一个有效的技能描述",
            "skillType": 0,
            "type": "Normal",
            "power": 50,
            "maxPP": 20,
            "priority": 8,
            "deletable": True
        }

        result = validator.validate_skill(valid_skill)
        self.assert_true(result, "有效技能验证", "应该通过验证")
        self.assert_true(len(validator.get_errors()) == 0, "错误列表", "应该为空")

    def test_validator_invalid_id(self):
        """测试验证器 - 无效ID"""
        self.print_separator("测试验证器 - 无效ID")

        validator = SkillValidator()

        # 负数ID
        invalid_skill = {
            "id": -1,
            "name": "测试",
            "description": "描述",
            "skillType": 0,
            "type": "Normal",
            "maxPP": 10
        }
        result = validator.validate_skill(invalid_skill)
        self.assert_false(result, "负数ID", "应该验证失败")

        # ID过大
        validator = SkillValidator()
        invalid_skill["id"] = 9999999
        result = validator.validate_skill(invalid_skill)
        self.assert_false(result, "ID过大", "应该验证失败")

    def test_validator_invalid_type(self):
        """测试验证器 - 无效属性类型"""
        self.print_separator("测试验证器 - 无效属性类型")

        validator = SkillValidator()
        invalid_skill = {
            "id": 1,
            "name": "测试",
            "description": "描述",
            "skillType": 0,
            "type": "InvalidType",  # 无效的属性类型
            "maxPP": 10
        }

        result = validator.validate_skill(invalid_skill)
        self.assert_false(result, "无效属性类型", "应该验证失败")
        errors = validator.get_errors()
        self.assert_true(len(errors) > 0, "错误信息", "应该包含错误")

    def test_validator_duplicate_ids(self):
        """测试验证器 - ID重复检查"""
        self.print_separator("测试验证器 - ID重复检查")

        validator = SkillValidator()
        skills = [
            {"id": 1, "name": "技能A", "description": "A", "skillType": 0, "type": "Normal", "maxPP": 5},
            {"id": 2, "name": "技能B", "description": "B", "skillType": 0, "type": "Fire", "maxPP": 10},
            {"id": 1, "name": "技能C", "description": "C", "skillType": 0, "type": "Water", "maxPP": 10},  # 重复ID
        ]

        is_valid, errors, warnings = validator.validate_batch(skills)
        self.assert_false(is_valid, "ID重复检测", "应该检测到重复ID")
        self.assert_true(any("Duplicate ID" in e for e in errors), "错误信息", "应该包含重复ID错误")

    def test_validator_path_traversal(self):
        """测试验证器 - 路径遍历防护"""
        self.print_separator("测试验证器 - 路径遍历防护")

        validator = SkillValidator()

        dangerous_paths = [
            "../../../etc/passwd",
            "..\\..\\windows\\system32",
            "/etc/passwd",
            "scripts/../../dangerous.lua"
        ]

        for path in dangerous_paths:
            skill = {
                "id": 1,
                "name": "测试",
                "description": "路径测试技能",
                "skillType": 0,
                "type": "Normal",
                "maxPP": 5,
                "scripterPath": path
            }
            result = validator.validate_skill(skill)
            self.assert_false(result, f"路径遍历防护 ({path})", "应该被拦截")

    # ==================== 导入器测试 ====================

    def test_importer_csv_basic(self):
        """测试导入器 - CSV基本导入"""
        self.print_separator("测试导入器 - CSV基本导入")

        test_csv = Path(__file__).parent / "test_data.csv"
        if not test_csv.exists():
            self.log("测试CSV文件不存在，跳过测试", "WARN")
            return

        output_dir = self.test_dir / "csv_import"
        importer = SkillImporter(output_dir=output_dir)

        result = importer.import_from_csv(test_csv)

        self.assert_true(result["success"], "CSV导入", "应该成功")
        self.assert_true(result["imported"] > 0, "导入数量", f"导入了 {result.get('imported', 0)} 个技能")

        # 检查输出文件
        json_files = list(output_dir.glob("*.json"))
        self.assert_true(len(json_files) > 0, "输出文件", f"生成了 {len(json_files)} 个JSON文件")

    def test_importer_file_size_limit(self):
        """测试导入器 - 文件大小限制"""
        self.print_separator("测试导入器 - 文件大小限制")

        # 创建一个小CSV文件
        small_csv = self.test_dir / "small.csv"
        with open(small_csv, 'w', encoding='utf-8') as f:
            f.write("id,name,description,skillType,type,maxPP\n1,测试,描述,0,Normal,5\n")

        output_dir = self.test_dir / "size_test"
        importer = SkillImporter(output_dir=output_dir)

        # 应该成功（文件很小）
        result = importer.import_from_csv(small_csv, max_size_mb=1)
        self.assert_true(result["success"], "小文件导入", "应该成功")

        # 设置极小的限制（0.00001 MB = 约10字节），应该失败
        result = importer.import_from_csv(small_csv, max_size_mb=0.00001)
        self.assert_false(result["success"], "超大小限制", "应该失败")
        error_msg = result.get("error", "").lower()
        self.assert_true("too large" in error_msg or "file too large" in error_msg,
                        "错误信息", f"应该提示文件过大，实际错误: {result.get('error', '')}")

    def test_importer_column_mapping(self):
        """测试导入器 - 列名映射"""
        self.print_separator("测试导入器 - 列名映射")

        # 创建使用中文列名的CSV
        chinese_csv = self.test_dir / "chinese.csv"
        with open(chinese_csv, 'w', encoding='utf-8') as f:
            f.write("技能ID,名称,描述,技能类型,属性类型,威力,最大PP,可删除,优先级\n")
            f.write("100,中文测试,这是测试描述,0,Fire,60,25,true,8\n")

        output_dir = self.test_dir / "chinese_import"
        importer = SkillImporter(output_dir=output_dir)

        result = importer.import_from_csv(chinese_csv)
        self.assert_true(result["success"], "中文列名导入", "应该成功")

        # 检查生成的JSON
        json_files = list(output_dir.glob("*.json"))
        if json_files:
            with open(json_files[0], 'r', encoding='utf-8') as f:
                data = json.load(f)
            self.assert_true(data["id"] == 100, "ID映射", "应该正确映射")
            self.assert_true(data["name"] == "中文测试", "名称映射", "应该正确映射")

    def test_importer_template_creation(self):
        """测试导入器 - 模板创建"""
        self.print_separator("测试导入器 - 模板创建")

        template_path = self.test_dir / "template.csv"
        importer = SkillImporter()

        result = importer.create_template_csv(template_path)

        self.assert_true(template_path.exists(), "模板文件", "应该被创建")

        # 验证模板内容
        with open(template_path, 'r', encoding='utf-8-sig') as f:
            lines = f.readlines()

        self.assert_true(len(lines) >= 2, "模板内容", "应该包含标题和示例行")
        self.assert_true("id" in lines[0].lower(), "模板标题", "应该包含id列")

    # ==================== 差异导出器测试 ====================

    def test_diff_exporter_basic(self):
        """测试差异导出器 - 基本功能"""
        self.print_separator("测试差异导出器 - 基本功能")

        # 创建两个版本的数据
        old_dir = self.test_dir / "old_skills"
        new_dir = self.test_dir / "new_skills"
        old_dir.mkdir(exist_ok=True)
        new_dir.mkdir(exist_ok=True)

        # 旧版本 - 3个技能
        old_skills = [
            {"id": 1, "name": "技能A", "type": "Normal", "power": 50},
            {"id": 2, "name": "技能B", "type": "Fire", "power": 70},
            {"id": 3, "name": "技能C", "type": "Water", "power": 60},
        ]

        for skill in old_skills:
            filename = f"{skill['id']:06d}_{skill['name']}.json"
            with open(old_dir / filename, 'w', encoding='utf-8') as f:
                json.dump(skill, f, ensure_ascii=False, indent=2)

        # 新版本 - 修改1个，删除1个，新增1个
        new_skills = [
            {"id": 1, "name": "技能A", "type": "Normal", "power": 55},  # 修改
            {"id": 2, "name": "技能B", "type": "Fire", "power": 70},     # 不变
            # 技能C被删除
            {"id": 4, "name": "技能D", "type": "Electric", "power": 80}, # 新增
        ]

        for skill in new_skills:
            filename = f"{skill['id']:06d}_{skill['name']}.json"
            with open(new_dir / filename, 'w', encoding='utf-8') as f:
                json.dump(skill, f, ensure_ascii=False, indent=2)

        # 执行差异比对
        exporter = SkillDiffExporter()
        report = exporter.export_diff_report(old_dir, new_dir)

        self.assert_true(report["summary"]["added"] == 1, "新增检测", "应该检测到1个新增")
        self.assert_true(report["summary"]["deleted"] == 1, "删除检测", "应该检测到1个删除")
        self.assert_true(report["summary"]["modified"] == 1, "修改检测", "应该检测到1个修改")
        self.assert_true(report["summary"]["unchanged"] == 1, "未变更检测", "应该检测到1个未变更")

    # ==================== 集成测试 ====================

    def test_full_workflow(self):
        """测试完整工作流程"""
        self.print_separator("测试完整工作流程")

        test_csv = Path(__file__).parent / "test_data.csv"
        if not test_csv.exists():
            self.log("测试CSV文件不存在，跳过集成测试", "WARN")
            return

        output_dir = self.test_dir / "full_workflow"

        # 1. 导入数据
        self.log("步骤 1: 导入CSV数据", "INFO")
        importer = SkillImporter(output_dir=output_dir)
        result = importer.import_from_csv(test_csv)

        self.assert_true(result["success"], "数据导入", "应该成功")

        # 2. 验证导出的JSON文件
        self.log("步骤 2: 验证导出的JSON文件", "INFO")
        json_files = list(output_dir.glob("*.json"))

        validator = SkillValidator()
        all_valid = True

        for json_file in json_files:
            with open(json_file, 'r', encoding='utf-8') as f:
                skill_data = json.load(f)
            if not validator.validate_skill(skill_data):
                all_valid = False
                break

        self.assert_true(all_valid, "JSON文件验证", "所有文件应该有效")

        # 3. 检查hash值
        self.log("步骤 3: 检查hash值", "INFO")
        has_hash = False
        for json_file in json_files:
            with open(json_file, 'r', encoding='utf-8') as f:
                skill_data = json.load(f)
            if "_meta" in skill_data and "hash" in skill_data["_meta"]:
                has_hash = True
                break

        self.assert_true(has_hash, "Hash生成", "应该生成hash值")

    # ==================== 性能测试 ====================

    def test_performance_bulk_validation(self):
        """测试性能 - 批量验证"""
        self.print_separator("测试性能 - 批量验证")

        # 生成1000个技能数据
        skills = []
        max_priority = SkillValidator.MAX_PRIORITY
        for i in range(1, 1001):
            skills.append({
                "id": i,
                "name": f"技能{i}",
                "description": f"技能{i}描述",
                "skillType": i % 3,
                "type": ["Normal", "Fire", "Water", "Electric"][i % 4],
                "power": (i * 10) % 200,
                "maxPP": (i * 5) % 50 + 1,
                "priority": i % (max_priority + 1),
                "deletable": i % 2 == 0,
                "guaranteedHit": i % 10 == 0
            })

        validator = SkillValidator()

        start_time = datetime.now()
        is_valid, errors, warnings = validator.validate_batch(skills)
        end_time = datetime.now()

        duration = (end_time - start_time).total_seconds()

        self.assert_true(is_valid, "批量验证", "应该全部通过")
        self.log(f"验证1000个技能耗时: {duration:.3f}秒", "INFO")
        self.assert_true(duration < 5.0, "性能测试", f"应该在5秒内完成 (实际: {duration:.3f}秒)")

    # ==================== 运行所有测试 ====================

    def run_all_tests(self):
        """运行所有测试"""
        self.print_separator("开始测试")
        self.log(f"测试输出目录: {self.test_dir}", "INFO")

        # 清理旧的测试文件
        self.cleanup()

        # 验证器测试
        self.test_validator_valid_skill()
        self.test_validator_invalid_id()
        self.test_validator_invalid_type()
        self.test_validator_duplicate_ids()
        self.test_validator_path_traversal()

        # 导入器测试
        self.test_importer_csv_basic()
        self.test_importer_file_size_limit()
        self.test_importer_column_mapping()
        self.test_importer_template_creation()

        # 差异导出器测试
        self.test_diff_exporter_basic()

        # 集成测试
        self.test_full_workflow()

        # 性能测试
        self.test_performance_bulk_validation()

        # 输出测试结果
        self.print_results()

    def print_results(self):
        """打印测试结果"""
        self.print_separator("测试结果")

        total = self.passed + self.failed
        pass_rate = (self.passed / total * 100) if total > 0 else 0

        print(f"\n总计: {total} 个测试")
        self.log(f"通过: {self.passed}", "PASS")
        if self.failed > 0:
            self.log(f"失败: {self.failed}", "FAIL")
        print(f"通过率: {pass_rate:.1f}%\n")

        # 显示失败的测试
        if self.failed > 0:
            self.log("失败的测试:", "FAIL")
            for test_name, passed in self.tests_run:
                if not passed:
                    print(f"  ✗ {test_name}")

        self.print_separator()

        # 返回退出代码
        return 0 if self.failed == 0 else 1


def main():
    """主函数"""
    print("""
╔══════════════════════════════════════════════════════════════╗
║        Rocoarena 技能数据管理工具 - 测试程序                 ║
╚══════════════════════════════════════════════════════════════╝
    """)

    runner = TestRunner()
    exit_code = runner.run_all_tests()

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
