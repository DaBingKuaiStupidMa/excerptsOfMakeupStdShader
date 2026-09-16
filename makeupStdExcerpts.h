/* 今附MIT.license分享 粉黛·正色 的内容节选, 献自 @大冰块stupid吗 */

/* 抗过曝核心:
* 对于[0,1)^3以内的色矢(r,g,b)
    * 存在标量k, 使得k*(r,g,b)中最大分量恰为1.0, k即为rgbMaxInv
    * 当色矢乘的倍数超过k时, 屏幕将超过1.0的分量被钳回1.0, 分量的占比发生改变, 饱和度和色调被损害
    * 许多光影在主世界故意施加超过分量超过1.0的光照, 这里需要结合k的性质, 提供一种防过曝重映射函数
* antiOverLyt的行为
    * a是主世界光照分量被允许达到的最大值, 不可逾越
    * 粉黛·正色 通过 lyt=mix(lyt, vec3(1.0), addLyt) 等手段, 限制主世界光照分量不超过1.4
    * 将x∈[1,a)光滑地单一映射到return∈[1,rgbMaxInv), 1处导数为1, a处导数为0, 是凸函数
    * 当且仅当a超过rgbMaxInv时候使用
        * 如果没超过, 鉴于rgbMaxInv的数学性质, 直接textureColor*=lytColor也不会损害色调和饱和度
        * 如果没超过, antiOverLyt实际上将光照效果提亮, 这不是合理行为
    * antiOverLyt是纯多项式, 次数为3 */
float antiOverLyt(float x, float rgbMaxInv){
    const float a =1.4; // worldExtraLyt
    const float A =a -1.0;
    const float invA =1.0 /A;
    const float invA2 =invA *invA;
    const float invA3 =invA2*invA;
    float t =x-1.0;
    float t2=t *t;
    float t3=t2*t;
    float p =t -2.0*invA*t2 +invA2*t3;
    float q =3.0*invA2*t2 -2.0*invA3*t3;
    return 1.0 +p +(rgbMaxInv-1.0)*q; // @大冰块stupid吗
}
/* 非标准饱和度:
* 衡量色矢的鲜艳程度, 只需检查它分量的方差, 方差为0, 意味着分量全同, 纯灰度的饱和度自认为0
* @楚隐霄 的新式矿物发光认为, 矿块上颜色鲜艳的部分是矿纹, 所以需要计算材质色的饱和度来判别
* 注意到方差在分量差异不大时, 数值按o(error^2)缩减, 可能损害分辨能力, 此外方差的计算略繁琐
* colorful的行为
    * 计算色矢分量的算术均值temp
    * 计算各分量与temp之差的绝对值总和
    * 返回值域为[0, 4/3], 当且仅当色矢形如(0,1,0), (1,0,1)时取最大值
* 奶油萌块具有特别泛黄的浅层岩石和淡蓝色的钻石矿纹, 经测定, 奶油萌块钻石矿
    * 岩石部分, colorful的最大值为48/256
    * 矿纹部分, colorful的最小值为108/256 */
float colorful(vec3 color){
    float temp=sumOfXYZ(color);
    temp*=(1.0/3.0);
    color=abs(color-vec3(temp));
    temp=sumOfXYZ(color); return temp;
}

/* 以下是renderchunk.fragment的内容, 为了阻挠学习, 亵渎开源精神
    * 恕不提供额外注释, 暂不公布其中其他函数的实现方式
    * 尽管如此, 这个架构也许具有一点启发性, 可以让AI帮助你理解架构的行为
* 大冰块认为
	* 公布光影的完整代码在这个时代更容易招来麻烦
	* 所以宁愿总是附MIT.license去发布一些 看起来 而且可能的确 无关紧要 的内容
	* 自己能力不足, 忌惮兼容性, 所以总是保留MOJANG代码的许多框架 */

// __multiversion__
// This signals the loading code to prepend either #version 100 or #version 300 es as apropriate.

#include "fragmentVersionCentroid.h"
#include "uniformShaderConstants.h"
#include "util.h"

#if __VERSION__ >= 300
  #ifndef BYPASS_PIXEL_SHADER
	#if defined(TEXEL_AA) && defined(TEXEL_AA_FEATURE)
	  _centroid in highp vec2 uv0;
	  _centroid in highp vec2 uv1;
	#else
	  _centroid in vec2 uv0;
	  _centroid in vec2 uv1;
	#endif
  #endif
#else
  #ifndef BYPASS_PIXEL_SHADER
	varying vec2 uv0;
	varying vec2 uv1;
  #endif
#endif // uv1.x是方块光照，uv1.y(<15/16)是含遮阴的天空光
#ifdef FOG
	varying vec4 fogColor;
#else
	vec4 fogColor=vec4(0.0);
#endif
LAYOUT_BINDING(0) uniform sampler2D TEXTURE_0;
LAYOUT_BINDING(1) uniform sampler2D TEXTURE_1;
LAYOUT_BINDING(2) uniform sampler2D TEXTURE_2;
uniform vec4 SUN_DIR;
uniform vec2 FOG_CONTROL;
uniform highp float TOTAL_REAL_WORLD_TIME;
#define t3600 TOTAL_REAL_WORLD_TIME
varying vec4 color, endDirStren;
varying vec3 EHW, worldXYZ, sunDir, viewDir, fakeNml, position, skyBase;
varying float snowMelt, endStren;
#include "globalLib.h"
void main(){
#ifdef BLEND
	//discard; // 便于检查河床
#endif
	// 绝对不要跨段使用临时变量的数值
	vec4 v4_temp; float temp; bool bl_temp;
#ifdef BYPASS_PIXEL_SHADER
	gl_FragColor=v0000;
#else // 持续到main()函数结尾

	vec3 skyColor=skyBase;
	// SUN_DIR.y不含瑕，有机会为负，在三四象限靠近横轴的方向跳反(5733.0/65536.0)
	vec2 SD_CORRE=(SUN_DIR.w>0.5 ? SUN_DIR.xy : -SUN_DIR.xy);
	// 天球上sunDir附近的正交基
	vec3 ePHI=vec3(-SD_CORRE.y, SD_CORRE.x, 0.0);
	vec3 eTHT=vec3(SD_CORRE, 1.732050808); // eTHT.z含*2.0修正
	bool dimEnd=EHW.x>0.5, dimHell =EHW.y>0.5, dimWld =EHW.z>0.5;
	// 视点位于水上，而不是像素位于水上。1.26用于排除细雪以下
	bool abuvWtr=FOG_CONTROL.x>0.01;
	bool smoothSnow=!abuvWtr && fogColor.r*1.26>fogColor.b;
	abuvWtr =abuvWtr ||smoothSnow;
	// 绝对不要使用SUN_DIR.w，因为SUN_DIR的跳变其实在地平线以下发生
	bool isDay=sunDir.y>0.0;
	// dayClear在晴天严格为1.0，雨天严格为0.0
	float dayClear=(abuvWtr && dimWld ? getDayClear(FOG_CONTROL.y) : 1.0);
	// uv1.y的数值不超过15/16，单层遮阴就必须挡下涟漪和水洼
	float shadCut=max(0.0, -14.0+16.0*uv1.y);
	float uv1x2=uv1.x*uv1.x;
	// 反常命名。数值越大，阳光越强，涉及不同天气中阴影的硬度
	float shadow=getShadow(uv1.y, dayClear);
	// 平滑昼夜正负，带符号晨昏因子(DawnDusk)
	float sgnDD=clamp(3.3 *sunDir.y, -1.0, 1.0);
	float absDD=abs(sgnDD);

	// 数值暂不明确则赋予良定义的初值
	vec4 txColor=v0000, AOBO=v1111;
	vec3 normal=v010, ripPud=v000, mirColor=v000, reflekt=v000;
	vec2 wave2D=vec2(0.0);
	bool isOre =false, useRainStuff =false, isBiome=false;
	float rgbMaxInv=1.0, oreLyt=0.0, rainStren=0.0, blk=0.0;

//#if USE_TEXEL_AA
	//txColor=texture2D_AA(TEXTURE_0, uv0);
//#else
	txColor=texture2D(TEXTURE_0, uv0);
//#endif
#ifdef SEASONS_FAR
	txColor.a = 1.0;
#endif
#if defined(BLEND)
	txColor.a *=color.a;
#endif
	AOBO=vec4(color.rgb, txColor.a); // 角落阴影*群系颜色(ambient_occlusion*biome_color)
#if !defined(BLEND) && ! USE_ALPHA_TEST
    isOre=(AOBO.g<1.0/256.0);
    if(isOre){
        if(maxOfRGB(txColor)>0.9){
            oreLyt=0.9; // 高光的饱和度很低，要单独识别
        }else{
            temp=colorful(txColor.rgb);
            temp=(temp-48.0/256.0) *(64.0/15.0); // 奶油萌块钻石矿[48,108)
            oreLyt=0.9*clamp(temp, 0.0, 1.0);
        }AOBO.g=AOBO.b; // 标记修正
    }
#endif // AOBO可能含夸张的颜色标记，所以isBiome必须在标记修正后判定
	isBiome =notGray(AOBO);
#ifdef FOG
	if(!smoothSnow){
		v4_temp=rainbow(viewDir, sunDir, dayClear, sgnDD);
		MIXTO(skyColor, v4_temp.rgb, v4_temp.a);
	}
#endif
#ifndef ALWAYS_LIT
	highp vec3 v_sigma=worldXYZ;	v_sigma=cross(dFdx(v_sigma), dFdy(v_sigma));
	v_sigma=normalize(v_sigma);		normal=v_sigma; // 高精度小灶
#endif
#if USE_ALPHA_TEST
	#ifdef ALPHA_TO_COVERAGE
		#define ALPHA_THRESHOLD 0.05
	#else
		#define ALPHA_THRESHOLD 0.5
	#endif
	if(txColor.a <ALPHA_THRESHOLD) discard; // 必须在法线计算结束之后，否则法线含瑕
#endif

#ifndef ALWAYS_LIT

	#if !defined(BLEND) && !USE_ALPHA_TEST
		// 自动法线认为，材质上暗处为凹，明处为凸
		temp=getGray(txColor); // center
		normal=txNormal(TEXTURE_0, uv0, TEXTURE_DIMENSIONS.xy, temp, normal, dayClear);
	#endif

	if(!dimHell){
	  wave2D=chunkWave(position.xz, t3600); // 在外计算是为方便水下焦散使用
	  vec3 turnXYZ, turnDir=v000; vec2 cosSinFy;
	  if(abuvWtr){ // 为了保护智力，禁绝水下镜效
		#ifdef BLEND
		// 水波和涟漪的返回值很大，振幅系数都是经验的
		  if(isBiome) normal.xz +=wave2D.xy *0.015;
		#endif
		  // 雨天，朝上，露天，并不下雪，dayClear含良定义
		  useRainStuff=(dayClear<1.0 && normal.y>0.7071 && uv1.y>14.0/16.0 && snowMelt>0.001);
		  if(useRainStuff){
			// 受到插值的影响，position不能认为是整数，同时为了协调耕地和草径上的水洼，+0.1
			ripPud =rainStuff(position.xz, t3600, uint(int(position.y+0.1))&15u);
			rainStren =(1.0-dayClear) *shadCut *snowMelt;
			ripPud *=rainStren; normal.xz +=ripPud.xy *0.191981;
		#ifdef BLEND
		  } // 半透明方块在晴天有镜像
		#endif
			turnXYZ		=worldXYZ -2.0 *dot(worldXYZ, normal) *normal;
			v4_temp.xyz	=turnXYZ.xyz *turnXYZ.xyz;
			temp		=v4_temp.x +v4_temp.z; // 为无破绽，只好精算turnDir
			cosSinFy	=turnXYZ.xz *inversesqrt(temp);
			turnDir		=turnXYZ.xyz*inversesqrt(temp +v4_temp.y);
			mirColor=(dimEnd ?
				getEndFog(turnDir, endDirStren, fogColor.rgb) :
				#ifdef BLEND
					getWorldFog(turnDir, sunDir, dayClear, sgnDD)
				#else
					simpRainFog(turnDir, sunDir, sgnDD)
				#endif
			);
			#ifdef BLEND // 水洼不含星空
				if(!isDay){ // stars
					float starLyt=starAlpha(turnDir, SD_CORRE, t3600);
					if(starLyt>0.01){
						temp =max(0.0,-0.1-sgnDD) *useDC(dayClear, 0.9) *starLyt;
						MIXTO(mirColor, vec3(0.7,0.9,1.0), temp);
					}
				}
			#endif
			if(dimWld){precision highp float; // sun, moon, discarded in the end
				temp=useDC(dayClear, 0.5);
				v4_temp.x=clamp(sgnDD+0.5, 0.0, 1.0)*temp;
				v4_temp.y=clamp(-sgnDD+0.5,0.0, 1.0)*temp;
				v4_temp=drawSquares(turnDir, sunDir, ePHI, eTHT, v4_temp.xy);
				MIXTO(mirColor, v4_temp.rgb, v4_temp.a);
				#ifdef BLEND // 水洼不含极光
					if(sgnDD<0.0 && turnDir.y>0.0){
						v4_temp.xz=-cosSinFy.yx; v4_temp.y =turnDir.y;
						mirColor=auroraIncluded(v4_temp.xyz, t3600, sgnDD, dayClear, mirColor);
					}
				#endif
			}if(turnDir.y>0.15){ // clouds, discarded out of fog
				vec3 cldColor; precision highp float;
				if(dimEnd){
					cldColor =mix(mirColor, vec3(0.8,0.8,0.5), 0.2);
					v4_temp =getEndCloud(turnXYZ, t3600, cldColor, turnDir.y, sunDir);
					MIXTO(mirColor, v4_temp.rgb, v4_temp.a);
				}else{
				  cldColor =mix(vec3(1.0), vec3(0.07, 0.21, 0.28), (isDay ?0.0 :-sgnDD));
				  cldColor =mix(cldColor, mirColor, 0.5); // 协调云色，以免突兀
					#ifdef BLEND
					  mirColor =dualCloudIncluded(
						turnXYZ, cosSinFy, t3600, cldColor, turnDir.y,
						sunDir, absDD, dayClear, mirColor
					  );
					#else
					  v4_temp =getRainyCloud(
						turnXYZ, cosSinFy, t3600, cldColor, turnDir.y, sunDir, absDD
					  );MIXTO(mirColor, v4_temp.rgb, v4_temp.a);
					#endif
				}
			}
			#ifdef BLEND
				if(sgnDD>-0.5 && dayClear<1.0){
					v4_temp=rainbow(turnDir, sunDir, dayClear, sgnDD);
					MIXTO(mirColor, v4_temp.rgb, v4_temp.a);
				}
			#endif
		#ifndef BLEND
		  } // 不透明方块在晴天无镜像
		#endif
	  }
	} // 至此，mirColor的计算终于结束

	v4_temp.rgb=AOBO.rgb;
	float removeFacing= // 剔除朝向亮度: 1.0 at y+, 0.8 at z, 0.6 at x, 0.5 at y-; hell: 0.9 at y+, 0.9 at y-
		abs(normal.x)>0.9 ? 5.0/3.0	:(
			abs(normal.z)>0.9 ? 1.25 :(
				abs(normal.y)<0.9 ? 1.0 :(
					dimHell ? 10.0/9.0 :(
						normal.y<0.0 ? 2.0 :1.0
	))));AOBO.rgb*=removeFacing;
	#ifdef BLEND
		bl_temp=isBiome;
	#else
		bl_temp=false;
	#endif
	// 修复向上透视树叶顶面，垂滴叶底面，水底看水面等错误
	AOBO.rgb =AOBO.g>1.1||bl_temp ?v4_temp.rgb :AOBO.rgb;

	bl_temp=abs(normal.y)<0.1 && abs(abs(normal.x)-abs(normal.z))<0.1; // 是草丛
	if(dimWld){
	  #ifndef BLEND
		if(bl_temp){ // 草丛亮度要与它扎根方块（顶面）和谐，所以单独算
			v4_temp.rgb =refleWld(sunDir, v010, viewDir, fakeNml, isDay, sgnDD, shadow, dayClear);
			reflekt =v4_temp.rgb *(1.0 +0.2 *dot((isDay?sunDir:-sunDir), normal) *absDD);
		}else reflekt =refleWld(sunDir, normal, viewDir, fakeNml, isDay, sgnDD, shadow, dayClear);
	  #else
	  	reflekt=useBlendMir(mirColor, sgnDD, uv1.y, dayClear);
	  #endif
	  	rgbMaxInv =maxOfRGB(txColor); // 计算时机须在角落阴影混入前
		rgbMaxInv =1.0 /rgbMaxInv; // 切勿覆值
	}else if(dimEnd){
	  #ifndef BLEND
		if(bl_temp){
			v4_temp.rgb =refleEnd(sunDir, endStren, v010, fakeNml);
			reflekt =v4_temp.rgb *(1.0 +0.2 *dot(sunDir, normal));
		}else reflekt =refleEnd(sunDir, endStren, normal, fakeNml);
	  #else
	  	reflekt=endBlendMir(mirColor);
	  #endif
	}else{ // dimHell, no mirSky
		if(bl_temp){
			v4_temp.rgb =refleHell(sunDir, fogColor.rgb, v010);
			reflekt =v4_temp.rgb *(1.0 +0.2 *dot(sunDir, normal));
		}else reflekt =refleHell(sunDir, fogColor.rgb, normal);
	}

	// 新架构认为txColor是吸收谱。可是，群系颜色作为吸收谱的一部分，
	// 却已经与角落阴影乘混在一起，这迫使代码进行一些本不必要的特别处理
	blk=max(oreLyt, uv1x2);
	#ifndef BLEND // BLEND没有角落阴影
		AOBO.rgb*=AOredeFactor(AOBO.g); // 略略加深暗角，群系颜色略微变暗，不影响SEASONS
		temp=maxOfRGB(AOBO);
		temp=1.0 /max(temp, 0.01); // 防黑
		AOBO.rgb=mix(AOBO.rgb, AOBO.rgb *temp, blk); // 方块照亮暗角
	#else
		if(isBiome) MIXTO(AOBO.rgb, vec3(0.9), 0.3);
	#endif
	// 角落阴影被方块光局部冲散后，编入吸收谱也可削弱reflekt，视觉能够接受
	#ifdef SEASONS
	// 阴险异常的bug，原因在于MOJANG对雪叶运用了专门colormap
		vec2 uv =color.xy;
		txColor.rgb *=mix(vec3(1.0,1.0,1.0), texture2D(TEXTURE_2, uv).rgb*2.0, color.b);
		temp=color.a;
		#if !defined(ALWAYS_LIT)
			temp*=removeFacing; if(temp>1.1) temp=color.a; // 修复向上透视树叶顶面的错误
		#endif
		txColor.rgb*=temp; txColor.a=1.0;
	#else
		txColor.rgb*=AOBO.rgb;
	#endif
	// 至此，群系颜色被混入txColor，吸收谱变得完整

	vec3 lytColor=reflekt;
	if(!abuvWtr && uv1.y<15.0/16.0){
		wave2D.y+=0.2*cos2pi(position.z*(7.0/16.0)-0.3*t3600); // 修瑕
		temp=dot(wave2D, wave2D); temp*=inversesqrt(temp); // length
		temp=max(0.0, 0.8-1.8*temp); // 劣质焦散
		temp*=uv1.y*(2.0-uv1.y); temp*=max(0.0, 0.3+0.4*normal.y);
		MIXTO(lytColor, vec3(1.0), temp);
	}
	vec3 blkLyt=useBlkLyt(uv1x2, oreLyt);
	MIXTO(lytColor, vec3(1.0), blkLyt);
	// 第三代节制过曝
	#ifndef BLEND
		if(dimWld){
			v4_temp.rgb=lytColor *1.4; // 1.4 extra lyt
			if(rgbMaxInv<1.4){
				temp=maxOfRGB(v4_temp);
				if(temp>1.0){
					temp=antiOverLyt(temp, rgbMaxInv)/temp;
					v4_temp.rgb*=temp;
				}
			}lytColor=v4_temp.rgb;
		}
	#endif
	txColor.rgb*=lytColor; // 乘吸收谱

	// 注意到水洼的背景来自方块，天空来自水反，水洼天空镜像应当按mix处理
	#ifndef BLEND
		if(useRainStuff){
			temp=clamp(ripPud.z,0.3,0.7) *rainStren;
			v4_temp.rgb =mix(txColor.rgb, mirColor.rgb, 0.65);
			txColor.rgb =mix(txColor.rgb, v4_temp.rgb, temp);
		} // rainStren已经包含各种环境限制
	#endif

#endif // IFNDEF ALWAYS_LIT

#ifdef FOG
	txColor.rgb = mix(txColor.rgb, skyColor, fogColor.a);
#endif
gl_FragColor=txColor; // @大冰块stupid吗
//if(gl_FragCoord.x>1280.0) {gl_FragColor=vec4(TEXTURE_DIMENSIONS.xy*(1.0/2048.0), 0.0, 1.0);}
#endif // BYPASS_PIXEL_SHADERs
}
