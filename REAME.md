
`cbch`是c block chain的缩写，是一个简单的区块链实现。

## 编译
在正式编译之前，先编译libvs的cryptopp库：
```shell
cd lib/cryptopp
make
```
将会在lib/cryptopp目录下生成libcryptopp.a文件。

然后编译cbch：
```shell
# cd到src所在目录
make
```
