import { hilog } from '@kit.PerformanceAnalysisKit';
import { constant } from '@kit.NearLinkKit';
import common from '@ohos.app.ability.common';
import process from '@ohos.process';
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
          text: `使用方法：在终端输入以下命令：

sh /dev/shm/aria2c --help

查看加载错误：
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
        let ret = napi.aria2_init(this.getNativeLibPath());
        this.message = ret ? 'aria2c进程已启动' : 'fork() 调用失败，无法启动 aria2c 进程';
      }, 0);
    })
  }
}
