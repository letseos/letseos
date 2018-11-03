# letseos

First Real Fair Game On EOS (第一款成功使用链上随机数的公平游戏)

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


# 分红
分红采用LET代币的形式,通过Bancor算法以EOS作为储备金保证其价值. LET 总
计1亿.通过前期玩游戏挖矿+Bancor交易获取.团队无预挖矿,合约自动发币保证
公平公正.

我们的优势
1. Bancor的价值会随着时间的增加而增加,永不破发. 玩家每玩一局游戏,其投
注额的3%将会进入LET的Bancor池.持有LET币就相当于您持有了整个项目的一部
分.不需要通过其他人购买您的币获益, 真正做到价值投资.

2. 总量恒定.团队无预挖.
LET币总量恒定: 100000000, 初期1000万个币10倍发送给初期参与者.

3. 自由流通
一旦送完1000万个LET,将会自动开启Bancor交易.

4. 价值高
每一个LET币将会价值 0.0074EOS!!!
真正做到玩游戏,拿分红.收益率>100%
