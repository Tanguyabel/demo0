#version 130

out vec4 vertexColor;
uniform vec2 resolution;

// camera settings
uniform vec3 origin;
uniform vec3 normal;
uniform vec3 u;
uniform vec3 v;
uniform float focal;

//objects
uniform int objNb;
uniform vec4 spheres[100]; // position and radius
uniform vec3 colors[100];
uniform vec3 attr[100];
  // attributes are, in that order:
  // diffusion, reflection, shininess (phong)

// lights
uniform float ambientLight;
uniform int lNb;
uniform vec4 lights[10]; // position and intensity

//vec2 p = -.5f + gl_FragCoord.xy / resolution.xy;
vec2 p = vec2(gl_FragCoord.x - resolution.x / 2, gl_FragCoord.y - resolution.y / 2);


float dot(vec3 u, vec3 v)
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

float intersect(vec3 o, vec3 dir, int i)
{
    vec3 dv = o - spheres[i].xyz;
    float sqrr = spheres[i].w * spheres[i].w;
    float delta = dot(dir, dv);
    delta *= delta;
    delta += -dot(dv, dv) + sqrr;

    if (delta < 0)
        return -1.0f;

    float d = dot(-dir, dv) - sqrt(delta);
    float D = dot(-dir, dv) + sqrt(delta);

    if (d > 0)
        return d;
    else if (D > 0)
        return D;
    else
        return -1.0f;
}

vec3 reflect(vec3 a, vec3 dir, vec3 n)
{
    if (dot(dir, n) > 0)
        n = -n;

    return 2 * dot(dir, n) * n - dir;
}

vec3 compColor(vec3 a, vec3 dir, vec3 inter, vec3 normal, vec3 s, int i)
{
    vec3 color = vec3(0,0,0);

    /*
      kd = attr[i].x;
      ks = attr[i].y;
      phong = attr[i].z;
     */

    for (int l = 0; l < lNb; ++l)
    {
        vec3 lDir = normalize(inter - lights[l].xyz);

        // compute shadow
        bool visible = true;
        for (int k = 0; k < objNb; ++k)
        {
            if (k == i)
                continue;
            float d = intersect(inter, -lDir, k);
            if (d > 0)
            {
                visible = false;
                k = objNb + 1; // break
            }
        }
        if (visible)
        {
            // compute diffusion
            float NdotL = max(dot(normal, lDir), 0.0f);
            color += attr[i].x * lights[l].w * colors[i] * NdotL;

            // compute specularity
            float SdotL = max(dot(s, lDir), 0.0f);
            color += attr[i].y * lights[l].w * colors[i] * pow(SdotL, attr[i].z);
        }
    }

    if (color.r > 1)
        color.r = 1;
    if (color.g > 1)
        color.g = 1;
    if (color.b > 1)
        color.b = 1;

    if (color.r < 0)
        color.r = 0;
    if (color.g < 0)
        color.g = 0;
    if (color.b < 0)
        color.b = 0;

    // ambient lighting
    color += ambientLight * colors[i];

    return color;
}

vec3 castRay(vec3 a_, vec3 dir_)
{
    float attenuationLimit = 10000;

    int curObj = -1;
    vec3 color = vec3(0f, 0f, 0f);

    vec3 a = a_;
    vec3 dir = dir_;
    float attenuation = 0;

    while (attenuation < attenuationLimit)
    {
        float d = 1e30;
        int o = -1;
        for (int i = 0; i < objNb; ++i)
        {
            if (i == curObj)
                continue;
            float d_ = intersect(a, dir, i);
            if (d_ > 0 && d_ < d)
            {
                d = d_;
                o = i;
            }
        }

        if (o >= 0)
        {
            attenuation += d;
            if (attenuation > attenuationLimit)
                break;

            // intersection point
            vec3 inter = a + d * dir;
            // object's normal at the intersection
            vec3 n = normalize(inter - spheres[o].xyz);
            // reflected ray
            vec3 s = reflect(a, dir, n);

            if (curObj == -1)
                color = compColor(a, dir, inter, n, s, o);
            else
                color += attr[curObj].y * compColor(a, dir, inter, n, s, o);
            if (attr[o].y == 0)
                break;
            else
            {
                curObj = o;
                a = inter;
                dir = -s;
            }
        }
        else
            break;
    }

    return color;
}

void main()
{
    // ray to launch from this pixel

    vec3 a = origin + p.x * u + p.y * v;
    vec3 dir = normalize(a - (origin - (focal * normal)));

    vertexColor = vec4(castRay(a, dir), 1f);
}
