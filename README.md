# letseos

First Real Fair Game On EOS (第一款成功使用链上随机数的公平游戏)

全部记录都在链上: https://www.myeoskit.com/account/letseosloger?sub=contract_interfaces

play: 您玩的记录.

result: 开奖记录.

如果有任何问题, 欢迎加入我们的电报群: http://t.me/letseosgame

## 公平:

letseos完全采用区块链上的随机数.在您向合约转账完成的同时, 您的开奖结果就已经确定了.

我们独创了这种不受玩家/区块生产者/项目方操控的随机数.

如果您有计算机基础可以自行验证自己的开奖结果.具体流程如下:

(如果您没有计算机基础, 但是对真实性有怀疑, 您可以查阅我们的开奖记录, 通过统计随机数出现次数的方式来验证公平性

```{\"reel0\":{\"pos\": 13 , \"number\": 5 },\"reel1\":{\"pos\": 3 , \"number\": 7 },\"reel2\":{\"pos\": 27 , \"number\": 0 },\"jackpot\":{ \"point\":0 , \"pos\":-1 , \"number\": -1 }}```
)

当您像合约转账时,我们会在RAM中记下您这次PLAY的信息,包括不重复的ID,金额,以及您提供的19位数字.

此时您的随机数种子就已经确定,公式是
​    sha256(id, block_time, seed)

- sign 是eos官方提供的签名方法
- block_time是您转账所在区块的ID
- seed 是您提供的19位随机数字
  因为我们不知道您提供的19位数字和block_time所以没有办法作弊.

接下来, 为了防止区块生产者作弊.我们会对上一步进行签名
​    sign(before_seed)

- 签名的私钥是我们提前生成的.对应的公约会公布在网站里. 除非遭受攻击,密钥泄漏. 我们不会更改公钥.
- 您可以通过我们提供的公钥验证签名结果.

最后一步就是使用签名结果作为随机数算法, 生成3个随机数.我们采用了MF1764, 生成均匀的随机数(我们不可能生成不随机的, 否则被黑客找到规律会被撸羊毛).

以上是我们的随机数生成过程, 我们花费了大量的精力保证随机数是真随机,真均匀.我们坚信区块链的力量,只有从区块链上产生的随机数, 才是真正公平的随机数.

this is how we create signature

```
func sign(id uint64, seed uint64, timestamp uint64) (ecc.Signature, error) {
	privkey, err := ecc.NewPrivateKey("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")
	util.PanicIfErr(err)

	//seed := random.String(64)
	sig, err := privkey.Sign(sigDigest(id, seed, timestamp))
	return sig, err
}

func sigDigest(id uint64, seed uint64, timestamp uint64) []byte {
	h := sha256.New()

	bs := make([]byte, 8, 8)
	binary.LittleEndian.PutUint64(bs, id)
	fmt.Println(bs)
	_, _ = h.Write(bs)
	binary.LittleEndian.PutUint64(bs, seed)
	_, _ = h.Write(bs)
	binary.LittleEndian.PutUint64(bs, timestamp)
	_, _ = h.Write(bs)
	return h.Sum(nil)
}
```

在我们的合约里也会对签名进行检查.

```
# 校验
sha256( bufDigest,  used , &digest);
assert_recover_key(&digest , (const char*)sig.data , sizeof(sig), pub_key.data, sizeof(pub_key));

# 生成随机数
const char *p64 = reinterpret_cast<const char *>(&sig.data);
uint32_t r = *(uint32_t*) &p64[12] 
random = MT19937.from_seed(r)
random.rand_u32()
```


### 什么是LET币

LET币是let's EOS社区的通证, 不同于其他通证,LET币是有自身价值锚定的.

### 价值来源

1. 玩家在LET平台上每玩一局游戏, 都会有3%流水到Bancor池中, 随着游戏进行, LET价格越来越高
2. 分红, LET平台取得的所有收益,都将通过分红的方式发送给所有持有LET币的用户 -- 正在开发. 预期是通过将LET币发送到官方合约中,每周统计快照以EOS发放
3. Bancor自由流通, 您可以随时交易.买入没有手续费, 卖出只有2%(防止薅羊毛).

### LET币流通情况

Bancor初始化的时候, 我们用 6000EOS 锚定了 3000万LET币, 每一个币的价格等于0.002.

初始化1000万的用途:

1. 创世挖矿: 3000万中的1000万用来返利用户, 第一期10倍返利(相当于平台补贴用户).

1. 团队锁仓: 1000万用于团队工资, 购买CPU, 锁仓期限2年, 通过合约逐步释放

1. 市值管理: 1000万用于管理LET币的市值, 和其他项目方合作, 或者发生重大意外由社区决定如何支配.

剩余7000万LET通过以下方式获得

1. Bancor交易
2. 玩游戏1倍返利

[LET币随着Bancor中EOS数量增加的变化](https://github.com/letseos/letseos/blob/master/LET_Bancor.png)
