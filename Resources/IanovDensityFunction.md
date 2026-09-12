float ResultNoise = 0;

float HeightGradient = (RayPos.z - ActorMin.z) / (ActorMax.z - ActorMin.z);

//Base Noise
float3 BaseNoiseUVW = (frac(RayPos / BaseNoiseTexTile + Wind));
float4 BaseNoise = BaseNoiseTex.SampleLevel(BaseNoiseTexSampler, BaseNoiseUVW, LOD);

//Small Noise
float3 SmallNoiseUVW = (frac(RayPos / SmallNoiseTexTile + Wind \* 10));
float4 SmallNoise = SmallNoiseTex.SampleLevel(SmallNoiseTexSampler, SmallNoiseUVW, LOD);

//Coverage texture
float4 WeatherMap = WeatherTex.SampleLevel(WeatherTexSampler, RayPos.xy / WeatherTexTile + Wind.xy \* 0.15, 0);

WeatherMap.b = saturate(WeatherMap.b + ((WeatherParam.r - 0.5) \* 2));

float WeatherState = max(WeatherMap.r, saturate(WeatherParam.r - 0.5) _ 2 _ WeatherMap.g);

float ShapeAltering = saturate(CustomExpression0(Parameters, HeightGradient, 0, 0.07, 0, 1)) _ saturate(CustomExpression0(Parameters, HeightGradient, WeatherMap.b _ 0.2, WeatherMap.b, 1, 0));

float DensityAltering = WeatherParam.g _ HeightGradient _ saturate(CustomExpression0(Parameters, HeightGradient, 0, 0.15, 0, 1)) _ saturate(CustomExpression0(Parameters, HeightGradient, 0.9, 1, 1, 0)) _ WeatherMap.a \* 2;

float BaseNoiseSample = CustomExpression0(Parameters, BaseNoise.r, (BaseNoise.g _ 0.625 + BaseNoise.b _ 0.25 + BaseNoise.a \* 0.125) - 1, 1, 0, 1);

float SmallNoiseSample = SmallNoise.r _ 0.625 + SmallNoise.g _ 0.25 + SmallNoise.b \* 0.125;

float DetailNoise = 0.35 _ exp(-WeatherParam.r _ 0.75) _ lerp(SmallNoiseSample, 1 - SmallNoiseSample, saturate(HeightGradient _ 10));

float ShapeNoise = saturate(CustomExpression0(Parameters, BaseNoiseSample _ ShapeAltering, 1 - WeatherParam.r _ WeatherState, 1, 0, 1));

ResultNoise = saturate(CustomExpression0(Parameters, ShapeNoise, DetailNoise, 1, 0, 1)) \* DensityAltering;

return ResultNoise;
