"""
技能数据差异导出工具
用于比较和导出技能数据的变更记录
"""

import os
import json
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from datetime import datetime
from hash_utils import hash_skillData, stable_jsonDumps


class SkillDiffExporter:
    """技能数据差异导出器"""

    def __init__(self, skills_dir: Optional[Path] = None, scripts_dir: Optional[Path] = None):
        """
        初始化差异导出器

        Args:
            skills_dir: 技能数据目录
            scripts_dir: 脚本目录
        """
        root_dir = Path(__file__).resolve().parents[2]
        base_dir = root_dir / "assets"
        self.skills_dir = skills_dir or (base_dir / "skills/skills_src").resolve()
        self.scripts_dir = scripts_dir or (base_dir / "skills/scripts").resolve()

        # 确保目录存在
        self.skills_dir.mkdir(parents=True, exist_ok=True)
        self.scripts_dir.mkdir(parents=True, exist_ok=True)

    def load_skill_json(self, skill_path: Path) -> Optional[Dict]:
        """
        加载技能JSON文件

        Args:
            skill_path: 技能文件路径

        Returns:
            技能数据字典，如果失败返回None
        """
        try:
            with open(skill_path, 'r', encoding='utf-8') as f:
                return json.load(f)
        except Exception as e:
            print(f"Error loading {skill_path}: {e}")
            return None

    def get_script_content(self, script_path: str) -> str:
        """
        获取脚本内容

        Args:
            script_path: 脚本相对路径

        Returns:
            脚本内容
        """
        if not script_path:
            return ""

        full_path = self.scripts_dir / script_path
        if not full_path.exists():
            return ""

        try:
            with open(full_path, 'r', encoding='utf-8') as f:
                return f.read()
        except Exception as e:
            print(f"Error reading script {script_path}: {e}")
            return ""

    def calculate_skill_hash(self, skill_data: Dict) -> str:
        """
        计算技能数据的hash值

        Args:
            skill_data: 技能数据

        Returns:
            Hash值
        """
        script_path = skill_data.get("scripterPath", "")
        script_content = self.get_script_content(script_path)
        return hash_skillData(skill_data, script_content)

    def compare_skills(self, old_skill: Dict, new_skill: Dict) -> Dict:
        """
        比较两个技能数据

        Args:
            old_skill: 旧技能数据
            new_skill: 新技能数据

        Returns:
            差异字典
        """
        changes = {}

        # 比较基本字段
        fields_to_compare = [
            "id", "name", "description", "skillType", "type",
            "power", "maxPP", "deletable", "priority", "scripterPath"
        ]

        for field in fields_to_compare:
            old_val = old_skill.get(field)
            new_val = new_skill.get(field)

            if old_val != new_val:
                changes[field] = {
                    "old": old_val,
                    "new": new_val
                }

        # 比较hash值
        old_hash = self.calculate_skill_hash(old_skill)
        new_hash = self.calculate_skill_hash(new_skill)

        if old_hash != new_hash:
            changes["_hash_changed"] = True

        return changes

    def scan_all_skills(self) -> Dict[int, Path]:
        """
        扫描所有技能文件

        Returns:
            技能ID到文件路径的映射
        """
        skills_map = {}

        for json_file in self.skills_dir.glob("*.json"):
            skill_data = self.load_skill_json(json_file)
            if skill_data and "id" in skill_data:
                skills_map[skill_data["id"]] = json_file

        return skills_map

    def export_diff_report(self, old_dir: Path, new_dir: Optional[Path] = None,
                          output_file: Optional[Path] = None) -> Dict:
        """
        导出差异报告

        Args:
            old_dir: 旧版本技能数据目录
            new_dir: 新版本技能数据目录（默认为当前目录）
            output_file: 输出报告文件路径

        Returns:
            差异报告
        """
        if new_dir is None:
            new_dir = self.skills_dir

        # 保存原始目录
        original_dir = self.skills_dir
        try:
            # 扫描旧版本
            self.skills_dir = old_dir
            old_skills = self.scan_all_skills()

            # 扫描新版本
            self.skills_dir = new_dir
            new_skills = self.scan_all_skills()

        finally:
            # 恢复原始目录
            self.skills_dir = original_dir

        # 分析差异
        report = {
            "timestamp": datetime.now().isoformat(),
            "old_dir": str(old_dir),
            "new_dir": str(new_dir),
            "summary": {
                "added": 0,
                "modified": 0,
                "deleted": 0,
                "unchanged": 0
            },
            "details": {
                "added": [],
                "modified": [],
                "deleted": []
            }
        }

        old_ids = set(old_skills.keys())
        new_ids = set(new_skills.keys())

        # 新增的技能
        added_ids = new_ids - old_ids
        for skill_id in added_ids:
            skill_data = self.load_skill_json(new_skills[skill_id])
            if skill_data:
                report["details"]["added"].append({
                    "id": skill_id,
                    "name": skill_data.get("name", ""),
                    "file": new_skills[skill_id].name
                })
        report["summary"]["added"] = len(added_ids)

        # 删除的技能
        deleted_ids = old_ids - new_ids
        for skill_id in deleted_ids:
            skill_data = self.load_skill_json(old_skills[skill_id])
            if skill_data:
                report["details"]["deleted"].append({
                    "id": skill_id,
                    "name": skill_data.get("name", ""),
                    "file": old_skills[skill_id].name
                })
        report["summary"]["deleted"] = len(deleted_ids)

        # 修改的技能
        common_ids = old_ids & new_ids
        for skill_id in common_ids:
            old_skill = self.load_skill_json(old_skills[skill_id])
            new_skill = self.load_skill_json(new_skills[skill_id])

            if old_skill and new_skill:
                changes = self.compare_skills(old_skill, new_skill)
                if changes:
                    report["details"]["modified"].append({
                        "id": skill_id,
                        "name": new_skill.get("name", ""),
                        "file": new_skills[skill_id].name,
                        "changes": changes
                    })
                    report["summary"]["modified"] += 1
                else:
                    report["summary"]["unchanged"] += 1

        # 输出报告
        if output_file:
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(report, f, ensure_ascii=False, indent=2)

        return report

    def print_report(self, report: Dict):
        """
        打印差异报告

        Args:
            report: 差异报告字典
        """
        print("\n" + "=" * 60)
        print("Skill Data Difference Report")
        print("=" * 60)
        print(f"Timestamp: {report['timestamp']}")
        print(f"Old: {report['old_dir']}")
        print(f"New: {report['new_dir']}")
        print("\nSummary:")
        print(f"  Added:     {report['summary']['added']}")
        print(f"  Modified:  {report['summary']['modified']}")
        print(f"  Deleted:   {report['summary']['deleted']}")
        print(f"  Unchanged: {report['summary']['unchanged']}")

        if report['details']['added']:
            print(f"\nAdded Skills ({len(report['details']['added'])}):")
            for skill in report['details']['added'][:10]:
                print(f"  + [{skill['id']}] {skill['name']}")
            if len(report['details']['added']) > 10:
                print(f"  ... and {len(report['details']['added']) - 10} more")

        if report['details']['deleted']:
            print(f"\nDeleted Skills ({len(report['details']['deleted'])}):")
            for skill in report['details']['deleted'][:10]:
                print(f"  - [{skill['id']}] {skill['name']}")
            if len(report['details']['deleted']) > 10:
                print(f"  ... and {len(report['details']['deleted']) - 10} more")

        if report['details']['modified']:
            print(f"\nModified Skills ({len(report['details']['modified'])}):")
            for skill in report['details']['modified'][:10]:
                print(f"  * [{skill['id']}] {skill['name']}")
                for field, change in skill['changes'].items():
                    if field != "_hash_changed":
                        print(f"      {field}: {change['old']} → {change['new']}")
            if len(report['details']['modified']) > 10:
                print(f"  ... and {len(report['details']['modified']) - 10} more")

        print("=" * 60 + "\n")


def main():
    """命令行入口"""
    import argparse

    parser = argparse.ArgumentParser(description="Export skill data differences")
    parser.add_argument("old_dir", help="Old version skill data directory")
    parser.add_argument("--new-dir", help="New version skill data directory (default: current)")
    parser.add_argument("--output", "-o", help="Output report file (JSON)")
    parser.add_argument("--quiet", "-q", action="store_true", help="Don't print report to console")

    args = parser.parse_args()

    exporter = SkillDiffExporter()

    old_dir = Path(args.old_dir)
    new_dir = Path(args.new_dir) if args.new_dir else None
    output_file = Path(args.output) if args.output else None

    report = exporter.export_diff_report(old_dir, new_dir, output_file)

    if not args.quiet:
        exporter.print_report(report)

    if output_file:
        print(f"Report saved to: {output_file}")


if __name__ == "__main__":
    main()
