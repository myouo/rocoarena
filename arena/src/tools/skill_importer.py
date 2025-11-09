"""
技能数据导入器
从Excel或CSV文件批量导入技能数据到JSON格式
"""

import os
import json
import csv
from pathlib import Path
from typing import Dict, List, Optional, Union
from datetime import datetime
from skill_validator import SkillValidator
from hash_utils import hash_skillData, stable_jsonDumps


class SkillImporter:
    """技能数据导入器"""

    def __init__(self, output_dir: Optional[Path] = None, script_dir: Optional[Path] = None):
        """
        初始化导入器

        Args:
            output_dir: JSON输出目录
            script_dir: Lua脚本目录
        """
        base_dir = Path(__file__).parent.parent / "assets/skills"
        self.output_dir = output_dir or (base_dir / "skills_src")
        self.script_dir = script_dir or (base_dir / "scripts")

        # 创建输出目录
        self.output_dir.mkdir(parents=True, exist_ok=True)

        # 初始化验证器
        self.validator = SkillValidator(script_base_dir=self.script_dir)

        # 列映射（Excel/CSV列名到JSON字段名）
        self.column_mapping = {
            "ID": "id",
            "id": "id",
            "技能ID": "id",
            "Name": "name",
            "name": "name",
            "名称": "name",
            "技能名称": "name",
            "Description": "description",
            "description": "description",
            "描述": "description",
            "技能描述": "description",
            "SkillType": "skillType",
            "skillType": "skillType",
            "技能类型": "skillType",
            "Type": "type",
            "type": "type",
            "属性": "type",
            "属性类型": "type",
            "Power": "power",
            "power": "power",
            "威力": "power",
            "MaxPP": "maxPP",
            "maxPP": "maxPP",
            "PP": "maxPP",
            "最大PP": "maxPP",
            "Deletable": "deletable",
            "deletable": "deletable",
            "可删除": "deletable",
            "Priority": "priority",
            "priority": "priority",
            "优先级": "priority",
            "先手值": "priority",
            "ScriptPath": "scripterPath",
            "scriptPath": "scripterPath",
            "scripterPath": "scripterPath",
            "脚本路径": "scripterPath",
        }

    def normalize_column_name(self, col_name: str) -> Optional[str]:
        """
        标准化列名

        Args:
            col_name: 原始列名

        Returns:
            标准化后的字段名，如果不在映射中则返回None
        """
        return self.column_mapping.get(col_name.strip())

    def import_from_csv(self, csv_path: Union[str, Path], validate: bool = True,
                       encoding: str = 'utf-8', max_size_mb: int = 50) -> Dict:
        """
        从CSV文件导入技能数据

        Args:
            csv_path: CSV文件路径
            validate: 是否进行验证
            encoding: 文件编码
            max_size_mb: 最大文件大小（MB）

        Returns:
            导入结果字典
        """
        csv_path = Path(csv_path)
        if not csv_path.exists():
            return {
                "success": False,
                "error": f"File not found: {csv_path}",
                "imported": 0
            }

        # 检查文件大小
        try:
            file_size_mb = csv_path.stat().st_size / (1024 * 1024)
            if file_size_mb > max_size_mb:
                return {
                    "success": False,
                    "error": f"File too large: {file_size_mb:.2f}MB (max: {max_size_mb}MB)",
                    "imported": 0
                }
        except OSError as e:
            return {
                "success": False,
                "error": f"Cannot access file: {str(e)}",
                "imported": 0
            }

        skills_data = []
        errors = []

        try:
            # 尝试使用指定编码读取
            encodings = [encoding, 'utf-8-sig', 'gbk', 'gb2312']
            content = None

            for enc in encodings:
                try:
                    with open(csv_path, 'r', encoding=enc, newline='') as f:
                        content = f.read()
                    encoding = enc
                    break
                except UnicodeDecodeError:
                    continue

            if content is None:
                return {
                    "success": False,
                    "error": f"Cannot decode file with any supported encoding",
                    "imported": 0
                }

            # 解析CSV
            reader = csv.DictReader(content.splitlines())

            for row_num, row in enumerate(reader, 2):  # 从第2行开始（第1行是标题）
                skill_data = {}

                for col_name, value in row.items():
                    if not col_name:
                        continue

                    field_name = self.normalize_column_name(col_name)
                    if field_name:
                        # 类型转换
                        skill_data[field_name] = self._convert_value(field_name, value)

                if skill_data:
                    skill_data['_source_row'] = row_num
                    skills_data.append(skill_data)

        except csv.Error as e:
            return {
                "success": False,
                "error": f"CSV format error: {str(e)}",
                "imported": 0
            }
        except UnicodeDecodeError as e:
            return {
                "success": False,
                "error": f"Encoding error (try --encoding gbk or utf-8-sig): {str(e)}",
                "imported": 0
            }
        except PermissionError as e:
            return {
                "success": False,
                "error": f"Permission denied: {str(e)}",
                "imported": 0
            }
        except Exception as e:
            import traceback
            return {
                "success": False,
                "error": f"Unexpected error: {str(e)}",
                "details": traceback.format_exc(),
                "imported": 0
            }

        # 验证和导出
        return self._process_and_export(skills_data, validate, csv_path.stem)

    def import_from_excel(self, excel_path: Union[str, Path], sheet_name: Union[str, int] = 0,
                         validate: bool = True, max_size_mb: int = 50) -> Dict:
        """
        从Excel文件导入技能数据

        Args:
            excel_path: Excel文件路径
            sheet_name: 工作表名称或索引
            validate: 是否进行验证
            max_size_mb: 最大文件大小（MB）

        Returns:
            导入结果字典
        """
        try:
            import pandas as pd
        except ImportError:
            return {
                "success": False,
                "error": "pandas library not installed. Install with: pip install pandas openpyxl",
                "imported": 0
            }

        excel_path = Path(excel_path)
        if not excel_path.exists():
            return {
                "success": False,
                "error": f"File not found: {excel_path}",
                "imported": 0
            }

        # 检查文件大小
        try:
            file_size_mb = excel_path.stat().st_size / (1024 * 1024)
            if file_size_mb > max_size_mb:
                return {
                    "success": False,
                    "error": f"File too large: {file_size_mb:.2f}MB (max: {max_size_mb}MB)",
                    "imported": 0
                }
        except OSError as e:
            return {
                "success": False,
                "error": f"Cannot access file: {str(e)}",
                "imported": 0
            }

        try:
            # 读取Excel文件
            df = pd.read_excel(excel_path, sheet_name=sheet_name)

            skills_data = []

            for idx, row in df.iterrows():
                skill_data = {}

                for col_name in df.columns:
                    if pd.isna(col_name):
                        continue

                    field_name = self.normalize_column_name(str(col_name))
                    if field_name:
                        value = row[col_name]
                        # 跳过NaN值
                        if pd.notna(value):
                            skill_data[field_name] = self._convert_value(field_name, value)

                if skill_data:
                    skill_data['_source_row'] = idx + 2  # Excel行号（从1开始，加上标题行）
                    skills_data.append(skill_data)

        except FileNotFoundError as e:
            return {
                "success": False,
                "error": f"File not found: {str(e)}",
                "imported": 0
            }
        except PermissionError as e:
            return {
                "success": False,
                "error": f"Permission denied: {str(e)}",
                "imported": 0
            }
        except ValueError as e:
            return {
                "success": False,
                "error": f"Invalid Excel file or sheet name: {str(e)}",
                "imported": 0
            }
        except Exception as e:
            import traceback
            return {
                "success": False,
                "error": f"Unexpected error reading Excel: {str(e)}",
                "details": traceback.format_exc(),
                "imported": 0
            }

        # 验证和导出
        return self._process_and_export(skills_data, validate, excel_path.stem)

    def _convert_value(self, field_name: str, value):
        """
        转换字段值到正确的类型

        Args:
            field_name: 字段名
            value: 原始值

        Returns:
            转换后的值
        """
        if value is None or (isinstance(value, str) and not value.strip()):
            # 为必需字段返回默认值
            defaults = {
                "id": 0,
                "name": "",
                "description": "",
                "skillType": 2,
                "type": "Normal",
                "power": 0,
                "maxPP": 0,
                "deletable": True,
                "priority": 8,
                "scripterPath": ""
            }
            return defaults.get(field_name, None)

        # 整数类型字段
        if field_name in ["id", "skillType", "power", "maxPP", "priority"]:
            try:
                return int(float(value))  # 先转float再转int，处理"1.0"这种情况
            except (ValueError, TypeError):
                return 0

        # 布尔类型字段
        if field_name == "deletable":
            if isinstance(value, bool):
                return value
            if isinstance(value, str):
                return value.lower() in ("true", "1", "yes", "是")
            return bool(value)

        # 字符串类型字段
        return str(value).strip()

    def _process_and_export(self, skills_data: List[Dict], validate: bool,
                           batch_name: str) -> Dict:
        """
        处理和导出技能数据

        Args:
            skills_data: 技能数据列表
            validate: 是否验证
            batch_name: 批次名称

        Returns:
            处理结果
        """
        if not skills_data:
            return {
                "success": False,
                "error": "No valid skill data found",
                "imported": 0
            }

        # 验证数据
        warnings = []
        if validate:
            is_valid, errors, warnings = self.validator.validate_batch(skills_data)
            if not is_valid:
                return {
                    "success": False,
                    "error": "Validation failed",
                    "errors": errors,
                    "warnings": warnings,
                    "imported": 0
                }

        # 导出为JSON文件
        imported_count = 0
        export_errors = []

        for skill in skills_data:
            try:
                # 移除内部字段
                source_row = skill.pop('_source_row', None)

                # 添加元数据
                skill['_meta'] = {
                    "imported_at": datetime.now().isoformat(),
                    "batch": batch_name,
                    "source_row": source_row
                }

                # 生成文件名
                skill_id = skill.get('id', 0)
                skill_name = skill.get('name', 'unknown')
                # 清理文件名中的非法字符
                safe_name = "".join(c for c in skill_name if c.isalnum() or c in (' ', '-', '_')).strip()
                filename = f"{skill_id:06d}_{safe_name}.json"

                output_path = self.output_dir / filename

                # 计算hash（如果有脚本）
                script_content = ""
                script_path = skill.get("scripterPath", "")
                if script_path:
                    full_script_path = self.script_dir / script_path
                    if full_script_path.exists():
                        with open(full_script_path, 'r', encoding='utf-8') as f:
                            script_content = f.read()

                skill['_meta']['hash'] = hash_skillData(skill, script_content)

                # 写入JSON文件
                with open(output_path, 'w', encoding='utf-8') as f:
                    json.dump(skill, f, ensure_ascii=False, indent=2)

                imported_count += 1

            except Exception as e:
                export_errors.append(f"Error exporting skill ID {skill.get('id', 'unknown')}: {str(e)}")

        result = {
            "success": imported_count > 0,
            "imported": imported_count,
            "total": len(skills_data),
            "output_dir": str(self.output_dir)
        }

        if warnings:
            result["warnings"] = warnings

        if export_errors:
            result["export_errors"] = export_errors

        return result

    def create_template_csv(self, output_path: Union[str, Path]):
        """
        创建CSV模板文件

        Args:
            output_path: 输出文件路径
        """
        headers = [
            "id", "name", "description", "skillType", "type",
            "power", "maxPP", "deletable", "priority", "scripterPath"
        ]

        example_row = {
            "id": "1",
            "name": "示例技能",
            "description": "这是一个示例技能描述",
            "skillType": "0",
            "type": "Normal",
            "power": "40",
            "maxPP": "35",
            "deletable": "true",
            "priority": "8",
            "scripterPath": "skills/example.lua"
        }

        with open(output_path, 'w', encoding='utf-8-sig', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=headers)
            writer.writeheader()
            writer.writerow(example_row)

        return str(output_path)


def main():
    """命令行入口"""
    import argparse

    parser = argparse.ArgumentParser(description="Import skill data from Excel/CSV files")
    parser.add_argument("input_file", help="Path to Excel or CSV file")
    parser.add_argument("--sheet", default=0, help="Excel sheet name or index (default: 0)")
    parser.add_argument("--no-validate", action="store_true", help="Skip validation")
    parser.add_argument("--encoding", default="utf-8", help="CSV file encoding (default: utf-8)")
    parser.add_argument("--output-dir", help="Output directory for JSON files")
    parser.add_argument("--create-template", help="Create a template CSV file")

    args = parser.parse_args()

    importer = SkillImporter(output_dir=Path(args.output_dir) if args.output_dir else None)

    # 创建模板
    if args.create_template:
        template_path = importer.create_template_csv(args.create_template)
        print(f"Template created: {template_path}")
        return

    # 导入数据
    input_path = Path(args.input_file)

    if input_path.suffix.lower() in ['.xlsx', '.xls']:
        result = importer.import_from_excel(input_path, sheet_name=args.sheet,
                                           validate=not args.no_validate)
    elif input_path.suffix.lower() == '.csv':
        result = importer.import_from_csv(input_path, validate=not args.no_validate,
                                         encoding=args.encoding)
    else:
        print(f"Error: Unsupported file format: {input_path.suffix}")
        return

    # 输出结果
    if result["success"]:
        print(f"✓ Successfully imported {result['imported']} skills")
        print(f"  Output directory: {result['output_dir']}")

        if "warnings" in result and result["warnings"]:
            print(f"\n⚠ Warnings ({len(result['warnings'])}):")
            for warning in result["warnings"][:10]:  # 只显示前10个
                print(f"  - {warning}")
            if len(result["warnings"]) > 10:
                print(f"  ... and {len(result['warnings']) - 10} more")
    else:
        print(f"✗ Import failed: {result.get('error', 'Unknown error')}")

        if "errors" in result:
            print(f"\nErrors ({len(result['errors'])}):")
            for error in result["errors"][:20]:  # 只显示前20个
                print(f"  - {error}")
            if len(result["errors"]) > 20:
                print(f"  ... and {len(result['errors']) - 20} more")


if __name__ == "__main__":
    main()
