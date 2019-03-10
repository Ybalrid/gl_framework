#pragma once

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chaiscript/chaiscript.hpp>
#include <string>
#include "transform.hpp"

static ModulePtr get_glm_module()
{

	//ddglm::pi<transform>;
	using namespace chaiscript;
	using namespace glm;
	using namespace std;
	ModulePtr glm_module = ModulePtr(new Module);

	////scalar
	glm_module->add(fun(static_cast<float (*)(float, float)>(max)), "max");
	glm_module->add(fun(static_cast<float (*)(float, float)>(min)), "min");
	glm_module->add(fun(static_cast<float (*)(float, float, float)>(clamp)), "clamp");
	glm_module->add(fun(static_cast<float (*)(float, float, float)>(mix)), "mix");
	glm_module->add(fun(static_cast<float (*)(float, float, float)>(mix)), "lerp");
	glm_module->add(fun(static_cast<float (*)(float)>(radians)), "radians");
	glm_module->add(fun(static_cast<float (*)(float)>(degrees)), "degrees");
	glm_module->add(fun(static_cast<float (*)(float)>(sin)), "sin");
	glm_module->add(fun(static_cast<float (*)(float)>(cos)), "cos");
	glm_module->add(fun(static_cast<float (*)(float)>(tan)), "tan");
	glm_module->add(fun(static_cast<float (*)(float)>(asin)), "asin");
	glm_module->add(fun(static_cast<float (*)(float)>(acos)), "acos");
	glm_module->add(fun(static_cast<float (*)(float)>(atan)), "atan");
	glm_module->add(fun(static_cast<float (*)(float)>(abs)), "abs");
	glm_module->add(fun(static_cast<float (*)(float)>(ceil)), "ceil");
	glm_module->add(fun(static_cast<float (*)(float)>(floor)), "floor");
	glm_module->add(fun(static_cast<float (*)(float)>(fract)), "fract");
	glm_module->add(fun(&glm::pi<float>), "pi");
	glm_module->add(fun(&glm::pi<float>), "half_tau");
	glm_module->add(fun(&glm::half_pi<float>), "half_pi");
	glm_module->add(fun(&glm::half_pi<float>), "quarter_tau");
	glm_module->add(fun(&glm::two_pi<float>), "two_pi");
	glm_module->add(fun(&glm::two_pi<float>), "tau");
	glm_module->add(fun(&glm::three_over_two_pi<float>), "three_over_two_pi");
	glm_module->add(fun(&glm::quarter_pi<float>), "quarter_pi");

	////3D vector
	glm_module->add(user_type<vec3>(), "vec3");
	glm_module->add(constructor<vec3(float)>(), "vec3");
	glm_module->add(constructor<vec3(float, float, float)>(), "vec3");
	glm_module->add(constructor<vec3(const vec3&)>(), "vec3");
	glm_module->add(constructor<vec3(const vec2&, float)>(), "vec3");
	glm_module->add(constructor<vec3()>(), "vec3");
	glm_module->add(fun(&vec3::x), "x");
	glm_module->add(fun(&vec3::y), "y");
	glm_module->add(fun(&vec3::z), "z");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(operator+)), "+");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(operator-)), "-");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&)>(operator-)), "-");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec3 (*)(float, const vec3&)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, float)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&)>(normalize)), "normalize");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(cross)), "cross");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&)>(reflect)), "reflect");
	glm_module->add(fun(static_cast<float (*)(const vec3&)>(length)), "length");
	glm_module->add(fun(static_cast<float (*)(const vec3&, const vec3&)>(dot)), "dot");
	glm_module->add(fun(static_cast<float (*)(const vec3&, const vec3&)>(distance)), "distance");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&, float)>(refract)), "refract");
	glm_module->add(fun(static_cast<vec3 (*)(const vec3&, const vec3&, float)>(mix)), "mix");
	glm_module->add(fun([](const vec3& a, const vec3& b) { return all(equal(a, b)); }), "==");
	glm_module->add(fun([](const vec3& v) -> std::string { return "vec3("
															   + to_string(v.x) + ", "
															   + to_string(v.y) + ", "
															   + to_string(v.z) + ")"; }), "to_string");

	////quaternion
	glm_module->add(user_type<quat>(), "quat");
	glm_module->add(constructor<quat()>(), "quat");
	glm_module->add(constructor<quat(const vec3&)>(), "quat");
	glm_module->add(fun(&quat::w), "w");
	glm_module->add(fun(&quat::x), "x");
	glm_module->add(fun(&quat::y), "y");
	glm_module->add(fun(&quat::z), "z");
	glm_module->add(fun(static_cast<quat (*)(const float&, const vec3&)>(angleAxis)), "angleAxis");
	glm_module->add(fun(static_cast<quat (*)(const quat&, const quat&)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec3 (*)(const quat&, const vec3&)>(operator*)), "*");
	glm_module->add(fun(static_cast<quat (*)(const quat&)>(conjugate)), "conjugate");
	glm_module->add(fun(static_cast<quat (*)(const quat&)>(inverse)), "inverse");
	glm_module->add(fun(static_cast<quat (*)(const quat&)>(normalize)), "normalize");
	glm_module->add(fun(static_cast<quat (*)(const quat&, const float&, const vec3&)>(rotate)), "rotate");
	glm_module->add(fun(static_cast<quat (*)(const quat&, const quat&, float)>(mix)), "mix");
	glm_module->add(fun(static_cast<quat (*)(const quat&, const quat&, float)>(mix)), "slerp");
	glm_module->add(fun([](const quat& a, const quat& b) { return all(equal(a, b)); }), "==");
	glm_module->add(fun([](const quat& q) -> std::string { return "quat("
															   + to_string(q.w) + ", "
															   + to_string(q.x) + ", "
															   + to_string(q.y) + ", "
															   + to_string(q.z) + ")"; }), "to_string");

	////2D vector
	glm_module->add(user_type<vec2>(), "vec2");
	glm_module->add(constructor<vec2(float)>(), "vec2");
	glm_module->add(constructor<vec2(float, float)>(), "vec2");
	glm_module->add(constructor<vec2(const vec2&)>(), "vec2");
	glm_module->add(constructor<vec2()>(), "vec2");
	glm_module->add(fun(&vec2::x), "x");
	glm_module->add(fun(&vec2::y), "y");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&)>(operator+)), "+");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&)>(operator-)), "-");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&)>(operator-)), "-");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec2 (*)(float, const vec2&)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, float)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&)>(normalize)), "normalize");
	//glm_module->add(fun(static_cast<vec2(*)(const vec2&, const vec2&)>(cross)), "cross");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&)>(reflect)), "reflect");
	glm_module->add(fun(static_cast<float (*)(const vec2&)>(length)), "length");
	glm_module->add(fun(static_cast<float (*)(const vec2&, const vec2&)>(dot)), "dot");
	glm_module->add(fun(static_cast<float (*)(const vec2&, const vec2&)>(distance)), "distance");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&, float)>(refract)), "refract");
	glm_module->add(fun(static_cast<vec2 (*)(const vec2&, const vec2&, float)>(mix)), "mix");
	glm_module->add(fun([](const vec2& a, const vec2& b) { return all(equal(a, b)); }), "==");
	glm_module->add(fun([](const vec2& v) -> std::string { return "vec2("
															   + to_string(v.x) + ", "
															   + to_string(v.y) + ", "
															   + ")"; }), "to_string");

	////4D vector
	glm_module->add(user_type<vec4>(), "vec4");
	glm_module->add(constructor<vec4(float)>(), "vec4");
	glm_module->add(constructor<vec4(float, float, float, float)>(), "vec4");
	glm_module->add(constructor<vec4(const vec4&)>(), "vec4");
	glm_module->add(constructor<vec4(const vec3&, float)>(), "vec4");
	glm_module->add(constructor<vec4(const glm::vec2&, float, float)>(), "vec4");
	glm_module->add(constructor<vec4()>(), "vec4");
	glm_module->add(fun(&vec4::x), "x");
	glm_module->add(fun(&vec4::y), "y");
	glm_module->add(fun(&vec4::z), "z");
	glm_module->add(fun(&vec4::w), "w");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&)>(operator+)), "+");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&)>(operator-)), "-");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&)>(operator-)), "-");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec4 (*)(float, const vec4&)>(operator*)), "*");
	//glm_module->add(fun(static_cast<vec4(*)(const vec4&, float)>(operator*)), "*");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&)>(normalize)), "normalize");
	//glm_module->add(fun(static_cast<vec4(*)(const vec4&, const vec4&)>(cross)), "cross");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&)>(reflect)), "reflect");
	glm_module->add(fun(static_cast<float (*)(const vec4&)>(length)), "length");
	glm_module->add(fun(static_cast<float (*)(const vec4&, const vec4&)>(dot)), "dot");
	glm_module->add(fun(static_cast<float (*)(const vec4&, const vec4&)>(distance)), "distance");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&, float)>(refract)), "refract");
	glm_module->add(fun(static_cast<vec4 (*)(const vec4&, const vec4&, float)>(mix)), "mix");
	glm_module->add(fun([](const vec4& a, const vec4& b) { return all(equal(a, b)); }), "==");
	glm_module->add(fun([](const vec4& v) -> std::string { return "vec4("
															   + to_string(v.x) + ", "
															   + to_string(v.y) + ", "
															   + to_string(v.z) + ", "
															   + to_string(v.w) + ")"; }), "to_string");

	return glm_module;
}

