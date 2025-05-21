#!/bin/bash

# 出错时自动退出
set -e

# version=v1.0-$(date +%Y%m%d)
version=v2.0

echo "📦 正在提交代码，版本号：$version"

git add .
git commit -m "新增用户鉴别删除操作$version"
git push origin main

echo "🏷️ 打标签 $version"
git tag $version
git push origin $version
echo "✅ 发布完成"
