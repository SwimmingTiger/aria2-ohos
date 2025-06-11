import { hilog } from '@kit.PerformanceAnalysisKit';
import { constant } from '@kit.NearLinkKit';
import { fileIo as fs } from '@kit.CoreFileKit';
import { common } from '@kit.AbilityKit';
import { util } from '@kit.ArkTS';
import napi from 'libentry.so';

const DOMAIN = 0x0000;

@Entry
@Component
struct Index {
  @State message: string = 'aria2c进程启动中...';

  // 动态获取原生库路径 /data/storage/el1/bundle/libs/arm64
  getNativeLibPath(): string {
    const context = getContext(this) as common.UIAbilityContext;
    return `${context.bundleCodeDir}/libs/arm64`;
  }

  async copyRawFile(name : string, dest : string) {
    try {
      const context = getContext(this) as common.UIAbilityContext;
      const rawFileContent = await context.resourceManager.getRawFileContent(name);
      const textDecoder = util.TextDecoder.create("utf-8");
      const str = textDecoder.decodeToString(rawFileContent);
      if (!napi.write_file(dest, str)) {
        throw new Error('napi write_file return false');
      }
    } catch (error) {
      hilog.error(DOMAIN, 'copyRawFile', "%{public}s %{public}s: %{public}s", name, dest, error);
    }
  }

  build() {
    Column() {
      Row() {
        Text(this.message)
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
          .width('100%')
          .height('100%')
          .textAlign(TextAlign.Center)
        }
        .width('80%')
        .height('10%')
      Row() {
        TextArea({
          text: `使用方法：

1. 打开终端，输入以下命令，把 aria2c 命令加载到终端：

source /dev/shm/aria2c

2. 运行 aria2c 命令：

aria2c --help
aria2c https://hu60.cn/tpl/classic/img/hulvlin3.png

3. 如果 aria2c 进程启动失败，查看错误信息：

cat /dev/shm/cn.hu60.aria2.stderr
cat /dev/shm/cn.hu60.aria2.stdout
`
        })
        .width('100%')
        .height('100%')
      }
      .width('80%')
      .height('90%')
    }
    .width('100%')
    .height('90%')
    .justifyContent(FlexAlign.Center)
    .onAppear(() => {
      // 不立即执行，防止阻塞UI线程
      setTimeout(() => {
        // 复制脚本文件
        this.copyRawFile('aria2c.sh', '/dev/shm/aria2c.sh');
        this.copyRawFile('aria2c', '/dev/shm/aria2c');

        // 启动 aria2c 进程
        let ret = napi.aria2_init(this.getNativeLibPath());
        this.message = ret ? 'aria2c进程已启动' : 'fork() 调用失败，无法启动 aria2c 进程';
      }, 0);
    })
  }
}
