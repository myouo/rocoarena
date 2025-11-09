#!/usr/bin/env python3
"""
技能数据管理工具快速演示
演示导入、验证、差异比对等功能
"""

import sys
from pathlib import Path

# 添加工具目录到路径
sys.path.insert(0, str(Path(__file__).parent))

from skill_importer import SkillImporter
from skill_validator import SkillValidator
from diff_exporter import SkillDiffExporter


def print_header(title):
    """打印标题"""
    print("\n" + "=" * 70)
    print(f"  {title}")
    print("=" * 70)


def demo_importer():
    """演示导入器"""
    print_header("演示 1: 从CSV导入技能数据")

    test_csv = Path(__file__).parent / "test_data.csv"
    if not test_csv.exists():
        print("❌ 测试CSV文件不存在")
        return

    # 创建导入器
    demo_output = Path(__file__).parent / "demo_output"
    importer = SkillImporter(output_dir=demo_output)

    print(f"\n📁 输入文件: {test_csv}")
    print(f"📁 输出目录: {demo_output}")

    # 执行导入
    print("\n⏳ 正在导入...")
    result = importer.import_from_csv(test_csv)

    # 显示结果
    if result["success"]:
        print(f"\n✅ 导入成功!")
        print(f"   导入数量: {result['imported']}/{result.get('total', result['imported'])}")
        print(f"   输出目录: {result['output_dir']}")

        if "warnings" in result and result["warnings"]:
            print(f"\n⚠️  警告 ({len(result['warnings'])}条):")
            for warning in result["warnings"][:3]:
                print(f"   - {warning}")
    else:
        print(f"\n❌ 导入失败: {result.get('error', '未知错误')}")

    # 列出生成的文件
    json_files = list(demo_output.glob("*.json"))
    if json_files:
        print(f"\n📄 生成的JSON文件 (前5个):")
        for json_file in json_files[:5]:
            print(f"   - {json_file.name}")
        if len(json_files) > 5:
            print(f"   ... 还有 {len(json_files) - 5} 个文件")


def demo_validator():
    """演示验证器"""
    print_header("演示 2: 验证技能数据")

    validator = SkillValidator()

    # 测试1: 有效的技能
    print("\n测试 1: 有效的技能数据")
    valid_skill = {
        "id": 1,
        "name": "示例技能",
        "skillType": 0,
        "type": "Fire",
        "power": 80,
        "maxPP": 15,
        "priority": 8,
        "deletable": True
    }

    if validator.validate_skill(valid_skill):
        print("✅ 验证通过")
    else:
        print("❌ 验证失败")
        for error in validator.get_errors():
            print(f"   错误: {error}")

    # 测试2: 无效的技能（ID过大）
    print("\n测试 2: 无效的技能数据 (ID过大)")
    invalid_skill = {
        "id": 9999999,  # 超过限制
        "name": "无效技能",
        "skillType": 0,
        "type": "Normal"
    }

    if validator.validate_skill(invalid_skill):
        print("✅ 验证通过")
    else:
        print("❌ 验证失败 (预期)")
        for error in validator.get_errors():
            print(f"   错误: {error}")

    # 测试3: 危险路径
    print("\n测试 3: 安全检查 (路径遍历攻击)")
    dangerous_skill = {
        "id": 1,
        "name": "危险技能",
        "skillType": 0,
        "type": "Normal",
        "scripterPath": "../../../etc/passwd"
    }

    if validator.validate_skill(dangerous_skill):
        print("❌ 验证通过 (不应该通过!)")
    else:
        print("✅ 已拦截 (安全检查通过)")
        for error in validator.get_errors():
            print(f"   错误: {error}")


def demo_diff_exporter():
    """演示差异导出器"""
    print_header("演示 3: 技能数据差异比对")

    # 检查是否有演示输出
    demo_output = Path(__file__).parent / "demo_output"
    if not demo_output.exists() or not list(demo_output.glob("*.json")):
        print("\n⚠️  请先运行演示1生成测试数据")
        return

    print("\n💡 在实际使用中，你可以比对两个版本的技能数据")
    print("   示例: python diff_exporter.py old_skills/ --new-dir new_skills/")
    print("\n📊 差异报告会包含:")
    print("   - 新增的技能")
    print("   - 删除的技能")
    print("   - 修改的技能 (字段级对比)")
    print("   - Hash值变化检测")


def demo_template():
    """演示模板创建"""
    print_header("演示 4: 创建CSV模板")

    template_path = Path(__file__).parent / "demo_template.csv"
    importer = SkillImporter()

    print(f"\n📝 创建模板文件: {template_path}")
    importer.create_template_csv(template_path)

    if template_path.exists():
        print("✅ 模板创建成功")
        print("\n模板内容预览:")
        with open(template_path, 'r', encoding='utf-8-sig') as f:
            lines = f.readlines()
            for i, line in enumerate(lines[:3], 1):
                print(f"   {i}. {line.rstrip()}")

        print("\n💡 你可以编辑这个模板文件，然后导入:")
        print(f"   python skill_importer.py {template_path}")
    else:
        print("❌ 模板创建失败")


def demo_performance():
    """演示性能"""
    print_header("演示 5: 性能测试")

    from datetime import datetime

    print("\n🚀 批量验证性能测试")
    print("   生成1000个技能数据...")

    # 生成1000个技能
    skills = []
    for i in range(1, 1001):
        skills.append({
            "id": i,
            "name": f"技能{i}",
            "skillType": i % 3,
            "type": ["Normal", "Fire", "Water"][i % 3],
            "power": (i * 10) % 200,
            "maxPP": (i * 5) % 50 + 1,
        })

    # 批量验证
    validator = SkillValidator()
    start = datetime.now()
    is_valid, errors, warnings = validator.validate_batch(skills)
    duration = (datetime.now() - start).total_seconds()

    print(f"\n✅ 验证完成")
    print(f"   数量: 1000 个技能")
    print(f"   耗时: {duration:.4f} 秒")
    print(f"   速度: {1000/duration:.0f} 个/秒")
    print(f"   结果: {'全部通过' if is_valid else '有错误'}")


def main():
    """主函数"""
    print("""
╔══════════════════════════════════════════════════════════════════════╗
║                                                                      ║
║          Rocoarena 技能数据管理工具 - 功能演示                        ║
║                                                                      ║
║  本演示将展示工具的主要功能:                                          ║
║  1. 从CSV导入技能数据                                                ║
║  2. 数据验证和安全检查                                               ║
║  3. 差异比对                                                         ║
║  4. 模板创建                                                         ║
║  5. 性能测试                                                         ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
    """)

    try:
        # 运行各个演示
        demo_importer()
        demo_validator()
        demo_diff_exporter()
        demo_template()
        demo_performance()

        # 总结
        print_header("演示完成")
        print("\n✨ 所有功能演示完成！")
        print("\n📚 更多信息:")
        print("   - 使用文档: tools.md")
        print("   - 测试说明: TEST_README.md")
        print("   - 安全报告: SECURITY_AND_OPTIMIZATION.md")
        print("\n🚀 快速开始:")
        print("   1. python skill_importer.py --create-template my_skills.csv")
        print("   2. 编辑 my_skills.csv 文件")
        print("   3. python skill_importer.py my_skills.csv")
        print("\n💡 运行测试:")
        print("   python test_tools.py")
        print()

    except KeyboardInterrupt:
        print("\n\n⚠️  演示被中断")
    except Exception as e:
        print(f"\n\n❌ 演示出错: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
