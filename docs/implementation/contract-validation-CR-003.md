# CR-003 契约与技术探针验证

日期：2026-08-03

## OpenAPI 契约

- 输入：`docs/design/api-contract-v4-CR-003.yaml`
- OpenAPI 版本头、11 个路径、18 个 Schema/引用均完成静态检查。
- 未定义 `$ref` 数量：0。
- 实现中的金额字段统一采用 integer minor units；库存游标保持字符串类型。
- 应用内部没有新增市场写入端点；库存访问仅使用 GET。

## WebView2 探针

- SDK：Microsoft.Web.WebView2 1.0.4078.44。
- 已纳入 `WebView2.h`、`WebView2EnvironmentOptions.h` 与 x64 `WebView2Loader.dll`。
- MinGW 13.1 最小编译探针通过；应用已成功链接 `ole32`、`uuid` 并产出可执行文件。
- WebView2 Runtime 不可用时显示明确错误，不降级为密码采集或非官方登录接口。

## 结论

契约与技术路线满足 G2 设计基线，可以进入集成测试。真实账号登录、Steam Guard 和官方市场页面行为保留到 G4 人工验证。
