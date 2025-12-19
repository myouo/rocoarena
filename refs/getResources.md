# 更多细节
 **以下均抓取自[洛克王国游戏官网](https://17roco.qq.com)** 
- 精灵资源0: https://res.17roco.qq.com/res/combat/spirits/编号-.swf
- 精灵图鉴资源链接1: http://res.17roco.qq.com/res/combat/previews/编号-idle.swf
- 精灵图鉴资源链接2: https://res.17roco.qq.com/res/combat/icons/编号-.png
   - 为仓库内未选择精灵样式
- 动态地图: https://res.17roco.qq.com/res/scene/地图编号/scene.swf
- BGM资源链接: http://res.17roco.qq.com/res/music/new_编号(01-103).swf
- 成长之路图标链接: https://res.17roco.qq.com/res/titile/编号(0-24).swf
- 守护兽图标链接: https://res.17roco.qq.com/res/guardianPet/100000101.swf
   - 101-108: 无皮肤守护兽
   - 201-208: 皮肤1
   - 301-308: 皮肤2
- 相关属性图标链接: https://res.17roco.qq.com/plugins/WorldMap/icons/00001.png
   - 00001-00017: 地图上可以捕捉的属性图标
   - 00018-00030: 异常状态图标(烧伤、寄生等)
   - 00031-00036: 6项种族值图标
- 孵出来的蛋蛋图片链接: https://res.17roco.qq.com/res/egg/编号(0-15、21、27-29).png
   - 8、9、11、14暂无,21为乖乖组蛋蛋,29为精灵组蛋蛋
- HP药剂图片链接: https://res.17roco.qq.com/res/item/编号(16842754-16842763、16842774、16842775).png
   - 含潘多诺格HP药剂
- PP药剂图片链接: https://res.17roco.qq.com/res/item/编号(16908290-16908296).png
   - 含潘多诺格PP药剂
- 异常状态药剂图片链接: https://res.17roco.qq.com/res/item/编号(16973826-16973833).png
- 咕噜球图片链接: https://res.17roco.qq.com/res/item/编号(17039362-17039376).png
   - 17039367为国王球;17039371暂无
- 经验果图片链接1: https://res.17roco.qq.com/res/item/编号(17170435-17170453,17170464-17170475).png
- 经验果图片链接2: https://res.17roco.qq.com/res/item/编号(17170481-17170487).png
   - 为1万经验果-魔方糖果
- 经验果图片链接3: https://res.17roco.qq.com/res/item/编号(17235972-17235975).png
   - 分别为`小成长果实`、`奇妙果`、`魔力果`、`怪蛋酱汁`
- 经验果图片链接4: https://res.17roco.qq.com/res/item/17498114.png
   - 为`升级果`~~(神奇糖果)~~
- 刷新天赋值果实图片链接: https://res.17roco.qq.com/res/item/编号(17367043-17367047).png
- 努力值果实图片链接: https://res.17roco.qq.com/res/item/编号(17563651-17563657).png
   - 17563651为遗忘果实
- 活动链接: https://res.17roco.qq.com/activity/编号/ui编号.swf（2024/4/20 当前编号为3674）
- 小游戏: https://res.17roco.qq.com/res/game/编号/game.swf（编号从2000-2200）


## 我想自己抓取资源
### 前提
分为两步: 提取和解析,您需要准备:  
1. [JPEXS Free Flash Decompiler](https://github.com/jindrapetrik/jpexs-decompiler/releases/latest)
2. 建议下载360浏览器,下述步骤均为360浏览器调试
### 提取
1. 进入洛克王国官网
2. 键盘按下F12
3. 选择[网络]
4. 进入游戏后进行操作(查看精灵等)
5. 在控制台会有相应`swf`或`xml`文件输出
6. 将`swf`文件下载(链接复制后在浏览器打开一个新页面粘贴后回车)
7. 将下载好的swf文件放入上述前提1的程序中即可
### 解析
以精灵图鉴资源链接为例,将`编号`改为`100`,[下载](http://res.17roco.qq.com/res/combat/previews/100-idle.swf)对应的精灵swf后,在  
1. 打开前提1的程序,选择打开下载的swf;  
![analyze_1.png](https://s2.loli.net/2022/09/10/YJX94jaSv7NLfHO.png)
2. 点击左边的`精灵`子项后可以看到精灵画面,鼠标右键,选择导出已选;  
![analyze_2.png](https://s2.loli.net/2022/09/10/pTmQU6NgOCDe9rc.png)
3. 保存为png;  
![analyze_3.png](https://s2.loli.net/2022/09/10/78bJOnzSWA2UrTy.png)

其他资源与之类似,您可以提取您想要的任何资源.例如,您想要提取背景音乐,保存swf后,找到对应的`子项`(可能是音频),选择保存为`.mp3`即可
