#!/bin/bash

# --- 配置区 ---
# 要安装的 APT 包名称 (请替换为你实际要安装的包名)
PACKAGE_NAME="libsqlite3-dev"

# --- 函数定义区 ---

# 检查当前系统是否为 Debian/Ubuntu 或兼容系统，并检查apt命令
check_prerequisites() {
    if ! command -v apt &> /dev/null; then
        echo "错误: 'apt' 命令未找到。" >&2
        echo "此脚本设计用于 Debian/Ubuntu 或兼容的 Linux 发行版。" >&2
        exit 1
    fi
}

# 更新包列表并安装包
install_package_globally() {
    echo "--- 准备全局安装 APT 包: $PACKAGE_NAME ---"
    
    # 1. 更新包列表
    echo "正在更新本地包列表..."
    # 使用 sudo 执行 apt update
    if ! sudo apt update; then
        echo "错误: apt update 失败。" >&2
        exit 1
    fi

    # 2. 安装指定的包
    echo "正在安装包: $PACKAGE_NAME"
    # 使用 sudo 执行 apt install。-y 参数表示自动回答 yes。
    if sudo apt install -y "$PACKAGE_NAME"; then
        echo "--- APT 包 $PACKAGE_NAME 全局安装成功！ ---"
    else
        echo "错误: APT 包 $PACKAGE_NAME 全局安装失败。请检查上面的错误信息。" >&2
        exit 1
    fi
}

# --- 主逻辑区 ---
check_prerequisites
install_package_globally

# 可选：验证安装
if command -v "$PACKAGE_NAME" &> /dev/null; then
    echo "验证成功: 命令 '$PACKAGE_NAME' 可以在 PATH 中找到。"
else
    # 对于有些不创建命令的库文件包，这条会失败，是正常的
    echo "提示: 包 $PACKAGE_NAME 可能不包含可执行命令，但安装已完成。"
fi