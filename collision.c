#include "sdl_udon_private.h"
#include "complex.h"


const double EPS = 1.0e-10;

static BOOL HitPointAndPoint(double selfPoints[], double targetPoints[], HitData * outHitData) {
    double ax = selfPoints[0], ay = selfPoints[1];
    double bx = targetPoints[0], by = targetPoints[1];
    return (fabs((ax - bx) + (ay - by)) < EPS) ? TRUE : FALSE;
}

static BOOL HitPointAndRect(double aPoints[], double bPoints[], HitData * outHitData) {
    double ax = aPoints[0], ay = aPoints[1];
    double rx1 = bPoints[0], ry1 = bPoints[1];
    double rx2 = bPoints[4], ry2 = bPoints[5];
    return ( ax >= rx1 && ax <= rx2 && ay >= ry1 && ay <= ry2 );
}

static BOOL HitPointAndLine(double selfPoints[], double targetPoints[], HitData * outHitData) {
    double x = selfPoints[0], y = selfPoints[1];
    double lx1 = targetPoints[0], ly1 = targetPoints[1];
    double lx2 = targetPoints[2], ly2 = targetPoints[3];

    double lineLength = SDL_sqrt( (lx2 - lx1)*(lx2 - lx1) + (ly2 - ly1)*(ly2 - ly1) );
    double pointLength = SDL_sqrt( (x - lx1)*(x - lx1) + (y - ly1)*(y - ly1) );
    double result = (lx2 - lx1)*(x - lx1) + (ly2 - ly1)*(y - ly1);
    
    return ((result-EPS < lineLength*pointLength) && (result+EPS > lineLength*pointLength) && 
        (lineLength >= pointLength));
}

static BOOL HitPointAndCircle(double aPoints[], double bPoints[], HitData * outHitData) {
    double ax = aPoints[0], ay = aPoints[1];
    double bx = bPoints[0], by = bPoints[1], bRadius = bPoints[2];
    return ((ax - bx)*(ax - bx) + (ay - by)*(ay - by)) <= bRadius*bRadius;
}

static double CalcInnerProduct(double vax, double vay, double vbx, double vby) { return vax * vbx + vay * vby; }
static double CalcCrossProduct(double vax, double vay, double vbx, double vby) { return vax * vby - vay * vbx; }

static inline BOOL HitCircleAndLine(double aPoints[], double bPoints[], HitData * outHitData) {
    double x = aPoints[0], y = aPoints[1], r = aPoints[2];
    double lx1 = bPoints[0], ly1 = bPoints[1], lx2 = bPoints[2], ly2 = bPoints[3];

    double vsX = lx2 - lx1, vsY = ly2 - ly1;
    double vaX = x - lx1, vaY = y - ly1;
    double vbX = x - lx2, vbY = y - ly2;

    outHitData->crossProduct = vsX*vaY-vaX*vsY;
    double d = fabs(outHitData->crossProduct) / SDL_sqrt((vsX*vsX+ vsY*vsY));

    if (d <= r) {
        if ((CalcInnerProduct(vaX, vaY, vsX, vsY) * CalcInnerProduct(vbX, vbY, vsX, vsY)) <= 0) { 
            return TRUE; 
        } else {
            if ( (r > SDL_sqrt((vaX*vaX+ vaY*vaY))) || (r > SDL_sqrt((vbX*vbX+ vbY*vbY))) ) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL HitCircleAndPolygon(double aPoints[],
    int bPointLen, double bPoints[], HitData *outHitData)
{
    double buffPoints[4];
    BOOL hitFlag = FALSE;
    BOOL insideHitFlag = TRUE;
    HitData childHitData;
    int z; for(z = 0; z < bPointLen; ++z) { p("%lf\n", bPoints[z]); }
    int i = 0; for (i = 0; i < bPointLen; i+=2) {
        buffPoints[0] = bPoints[i];buffPoints[1] = bPoints[i+1];
        buffPoints[2] = bPoints[(i+2)%bPointLen];
        buffPoints[3] = bPoints[(i+3)%(bPointLen-1)];
        p("%lf : %lf : %lf : %lf\n", buffPoints[0],  buffPoints[1], buffPoints[2], buffPoints[3]);
        if (HitCircleAndLine(aPoints, buffPoints, &childHitData)) {
            hitFlag = TRUE;
            break;
        } else {
            if (childHitData.crossProduct > 0) { insideHitFlag = FALSE; }
        }
    }
    return (hitFlag || insideHitFlag);
}
static BOOL HitLineAndLine(double *aPoints, double * bPoints, HitData *outHitData) {
    double ax1 = aPoints[0], ay1 = aPoints[1];
    double ax2 = aPoints[2], ay2 = aPoints[3];
    double avx = ax2 - ax1, avy = ay2 - ay1;

    double bx1 = bPoints[0], by1 = bPoints[1];
    double bx2 = bPoints[2], by2 = bPoints[3];
    double bvx = bx2 - bx1, bvy = by2 - by1;

    double cross_av_bv = CalcCrossProduct(avx, avy, bvx, bvy);
    if (cross_av_bv == 0.0f) { return FALSE; } 

    double vx = bx1 - ax1, vy = by1 -ay1;

    double cross_v_av = CalcCrossProduct(vx, vy, avx, avy);
    double cross_v_bv = CalcCrossProduct(vx, vy, bvx, bvy);
    double t1 = cross_v_bv / cross_av_bv;
    double t2 = cross_v_av / cross_av_bv;
    if (t1 + EPS < 0 || t1 - EPS > 1 || t2 + EPS < 0 || t2 - EPS > 1)     {
        return FALSE;
    }
    outHitData->intersectX = ax1 + avx * t1;
    outHitData->intersectY = ay1 + avy * t1;
    return TRUE;
}

static BOOL HitPointAndPolygon(double *aPoints, int bPointLen, double * bPoints, HitData *outHitData) {
    BOOL hitFlag = FALSE;
    HitData childHitData;
    double buffPoints[4];
    double sum = 0.0f;
    int i;for (i = 0; i < bPointLen; i+=2) {
        buffPoints[0] = bPoints[i];buffPoints[1] = bPoints[i+1];
        buffPoints[2] = bPoints[(i+2)%bPointLen];
        buffPoints[3] = bPoints[(i+3)%bPointLen];
        if (HitPointAndLine(aPoints, buffPoints, &childHitData)) {
            hitFlag = TRUE; break;
        }
        double complex complex_p = aPoints[0] + aPoints[1] * I;
        double complex complex_l1 = buffPoints[0] + buffPoints[1] * I;
        double complex complex_l2 = buffPoints[2] + buffPoints[3] * I;

        sum += carg((complex_l2 - complex_p ) / (complex_l1 - complex_p ));
    }
    return hitFlag || (abs(sum) > 1);
}


static void HitIncludeLineInPolygon(double *aPoint, double *bPoint, double *lower, double *upper) {
    double x1 = aPoint[0], y1 = aPoint[1];
    double x2 = aPoint[2], y2 = aPoint[3];
    double lx1 = bPoint[0], ly1 = bPoint[1];
    double lx2 = bPoint[2], ly2 = bPoint[3];
    double complex complex_pt1 =  x1 + y1 * I;
    double complex complex_pt2  = x2 + y2 * I;
    double complex complex_l1 =  lx1 + ly1 * I;
    double complex complex_l2  = lx2 + ly2 * I;

    double complex dir0 = complex_pt2 - complex_pt1; 
    double complex ofs = complex_l1 - complex_pt1;
    double complex dir1 = complex_l2 - complex_l1;
    double num = cimag(conj(ofs)*dir1);
    double denom = cimag(conj(dir0)*dir1);
    if (abs(denom) < EPS) {
        if (num < EPS) { 
            *lower = *upper = 0.0; 
        } 
    } else {
        double t = num / denom;
        if (denom > 0) {
            if (*upper < t) { *upper = t; }
        } else {
            if (*lower > t) { *lower = t; }
        }
    }
}

static inline BOOL IsLeftHandedVertex(int bPointLen, double *bPoints) {
    double result = 0.0f;
    int i; for (i = 0; i < (bPointLen-2); i+=2) {
        double x1 = bPoints[i], y1 = bPoints[i+1];
        double x2 = bPoints[(i+2)%bPointLen];
        double y2 = bPoints[(i+3)%bPointLen];
        double x3 = bPoints[(i+4)%bPointLen];
        double y3 = bPoints[(i+5)%bPointLen];

        double a = atan2(y3-y2, x3 -x2) - atan2(y2-y1, x2-x1);
        if (a > MATH_PI) a -= MATH_DOUBLE_PI;
        if (a < -MATH_PI) a += MATH_DOUBLE_PI;
        result += a;
    }
    if (result >= MATH_DOUBLE_PI - 0.000001f &&
        result <= MATH_DOUBLE_PI + 0.000001f) {
        return TRUE;
    }
        return FALSE;
}

static void SortVertexLeftHanded(int pointLen, double points[], double outSortPoints[]) {
        outSortPoints[0] = points[0]; outSortPoints[1] = points[1];
        int i; for (i = 2; i <= pointLen-2; i+=2) {
            outSortPoints[i] = points[pointLen-i];
            outSortPoints[i+1] = points[pointLen-i+1];
        }
}

static BOOL HitLineAndPolygon(double *aPoints, int bPointLen, double * bPoints, HitData *outHitData) {
    int i = 0;
    double *bPointHead = bPoints;
    if (!IsLeftHandedVertex(bPointLen, bPoints)) {
        double sortBPoint[8];
        SortVertexLeftHanded(bPointLen, bPoints, sortBPoint);
        bPointHead = sortBPoint;
    }
    double buffPoints[4];
    BOOL hitFlag = FALSE;
    double lower = 0.0, upper = 1.0;
    HitData childHitData;
    for (i = 0; i < bPointLen; i+=2) {
        buffPoints[0] = bPointHead[i];buffPoints[1] = bPointHead[i+1];
        buffPoints[2] = bPointHead[(i+2)%bPointLen];
        buffPoints[3] = bPointHead[(i+3)%bPointLen];
        if (HitLineAndLine(aPoints, buffPoints, &childHitData)) {
            hitFlag = TRUE; break;
        } else {
            HitIncludeLineInPolygon(aPoints, buffPoints, &lower, &upper);
        }
    }
    p("h %d : u %lf - l %lf = ul %lf\n",hitFlag, upper, lower,  upper-lower);
    return (hitFlag) || ( 1.0 >= (upper-lower) && (upper - lower) > EPS) ;
}

static BOOL HitPolygonAndPolygon(int aPointLen, double *aPoints, 
    int bPointLen, double * bPoints, HitData *outHitData) {
    int i, j;
    double *aPointHead = aPoints, *bPointHead = bPoints;
    if (!IsLeftHandedVertex(aPointLen, aPoints)) {
        double sortAPoint[8];
        SortVertexLeftHanded(aPointLen, aPoints, sortAPoint);
        aPointHead = sortAPoint;
    }
    if (!IsLeftHandedVertex(bPointLen, bPoints)) {
        double sortBPoint[8];
        SortVertexLeftHanded(bPointLen, bPoints, sortBPoint);
        bPointHead = sortBPoint;
    }
    double aBuffPoints[4], bBuffPoints[4];
    BOOL hitFlag = FALSE;
    double aLower = 0.0, aUpper = 1.0, bLower = 0.0, bUpper = 1.0;
    HitData childHitData;
    double includeLineBuff[4];
    for (i = 0; i < aPointLen; i+=2) {
        aBuffPoints[0] = aPointHead[i];aBuffPoints[1] = aPointHead[i+1];
        aBuffPoints[2] = aPointHead[(i+2)%aPointLen];
        aBuffPoints[3] = aPointHead[(i+3)%aPointLen];
        for (j = 0; j < bPointLen; j+=2) {
            bBuffPoints[0] = bPointHead[j];bBuffPoints[1] = bPointHead[j+1];
            bBuffPoints[2] = bPointHead[(j+2)%bPointLen];
            bBuffPoints[3] = bPointHead[(j+3)%bPointLen];
            if (HitLineAndLine(aBuffPoints, bBuffPoints, &childHitData)) {
                hitFlag = TRUE; break;
            } else {
                includeLineBuff[0] = aBuffPoints[2]; includeLineBuff[1] = aBuffPoints[3];
                includeLineBuff[2] = aBuffPoints[0]; includeLineBuff[3] = aBuffPoints[1];
                HitIncludeLineInPolygon(includeLineBuff, bBuffPoints, &aLower, &aUpper);
                includeLineBuff[0] = bBuffPoints[2]; includeLineBuff[1] = bBuffPoints[3];
                includeLineBuff[2] = bBuffPoints[0]; includeLineBuff[3] = bBuffPoints[1];
                HitIncludeLineInPolygon(includeLineBuff, aBuffPoints, &bLower, &bUpper);
            }
        }
        if (hitFlag) break;
    }
    return (hitFlag) || ( 1.0f >= (aUpper-aLower) && (aUpper - aLower) > EPS) ||
    ( 1.0f >= (bUpper-bLower) && (bUpper - bLower) > EPS);
}

static BOOL HitRectAndRect(double *aPoints, double * bPoints, HitData *outHitData) {
    int i,j;
    double ax = aPoints[0], ay = aPoints[1], ax2 = aPoints[4], ay2 = aPoints[5];
    double bx = bPoints[0], by = bPoints[1], bx2 = bPoints[4], by2 = bPoints[5];
    BOOL hitFlag = FALSE;
    for (i = 0; i < 8; i+=2) {
        double ix = aPoints[i], iy = aPoints[i+1];
        if (bx <= ix && bx2 >= ix && by <= iy && by2 >= iy) { hitFlag = TRUE; break; }
        for (j = 0; j < 8; j+= 2) {
            double jx = bPoints[i], jy = bPoints[i+1];
            if (ax <= jx && ax2 >= jx && ay <= jy && ay2 >= jy) { hitFlag = TRUE; break; }
        }
        if (hitFlag) break;
    }
    return hitFlag;
}

static BOOL HitCircleAndCircle(double *selfPoints, double * targetPoints, HitData *outHitData) {
    double self_x, self_y, self_r;
    double target_x, target_y, target_r;
    double a, b;
    self_x = selfPoints[0]; self_y = selfPoints[1]; self_r = selfPoints[2];
    target_x = targetPoints[0]; target_y = targetPoints[1]; target_r = targetPoints[2];

    a = (self_x - target_x) * (self_x - target_x);
    b =(self_y - target_y) * (self_y - target_y);
    return ( (a + b) <= (self_r + target_r) * (self_r + target_r) ) ? TRUE : FALSE;
}

void AddNextHitObjectData(HitObjectData* currentHitObjectData, CollisionArea* aArea, int aIndex,
    CollisionArea* bArea, int bIndex, HitData *hitData)
{
    currentHitObjectData->next = malloc(sizeof(HitObjectData));
    currentHitObjectData = currentHitObjectData->next; currentHitObjectData->next = NULL;
    currentHitObjectData->selfArea = aArea; currentHitObjectData->targetArea = bArea;
    currentHitObjectData->selfIndex = aIndex; currentHitObjectData->targetIndex = bIndex;
}

static VALUE collision_init_collision(int argc, VALUE argv[], VALUE self) {
    VALUE hit_area_class = rb_path2class("SDLUdon::Collision::Area");
    rb_ivar_set(self, rb_intern("@hit_area"), rb_class_new_instance(argc, argv, hit_area_class));
    return Qnil;
}

static VALUE collision_get_hit_area(VALUE self) { return rb_ivar_get(self, rb_intern("@hit_area")); }

static VALUE collision_collidable(VALUE self, VALUE flag) {
    VALUE hit_area = rb_ivar_get(self, rb_intern("@hit_area"));
    CollisionArea* areaData; Data_Get_Struct(hit_area, CollisionArea, areaData);
    areaData->active = (flag == Qtrue) ? TRUE : FALSE;
    return self;
}

static VALUE collision_is_collied(VALUE self) {
    VALUE hit_area = rb_ivar_get(self, rb_intern("@hit_area"));
    CollisionArea* areaData; Data_Get_Struct(hit_area, CollisionArea, areaData);
    return (areaData->active) ? Qtrue : Qfalse;
}

static void area_dfree(CollisionArea *area)  {
    CollisionArea *buff = area;
    while (buff != NULL) {
        if (buff->points != NULL) { free(buff->points); }
        if (buff->key != NULL) { free(buff->key); }
        CollisionArea* old = buff; buff = buff->next; free(old);
    }
}

static VALUE area_alloc(VALUE klass) {
    CollisionArea *area = ALLOC(CollisionArea);
    return Data_Wrap_Struct(klass, 0, area_dfree, area);
}

CollisionArea* CollisionParseFromArray(VALUE ary) {
    int i, len = RARRAY_LENINT(ary);
    if (len == 0) { return NULL; }
    CollisionArea* area = malloc(sizeof(CollisionArea));
    area->pointLen = len; 
    area->active = TRUE;
    area->key = NULL;
    area->next = area->prev = NULL;
    double x, y, x2, y2;

    switch (len) {
        case 2:
            area->type = COLLISION_AREA_POINT;
            area->points = malloc(sizeof(double) * len);
            area->points[0] = NUM2DBL(RARRAY_PTR(ary)[0]);
            area->points[1] = NUM2DBL(RARRAY_PTR(ary)[1]);
            break;
        case 3:
            area->type = COLLISION_AREA_CIRCLE;
            area->points = malloc(sizeof(double) * len);
            for (i = 0; i < len; ++i) { area->points[i] = NUM2DBL(RARRAY_PTR(ary)[i]); }
            break;
        case 4:
            area->type = COLLISION_AREA_RECT;
            area->pointLen = 8;
            area->points = malloc(sizeof(double) * area->pointLen);
            area->points[0] = NUM2DBL(RARRAY_PTR(ary)[0]); 
            area->points[1] = NUM2DBL(RARRAY_PTR(ary)[1]); 
            area->points[2] = NUM2DBL(RARRAY_PTR(ary)[2]); 
            area->points[3] = area->points[1]; 
            area->points[4] = area->points[2];
            area->points[5] = NUM2DBL(RARRAY_PTR(ary)[3]); 
            area->points[6] = area->points[0];
            area->points[7] = area->points[5];
            break;
        case 6:
            x = NUM2DBL(RARRAY_PTR(ary)[0]);
            y = NUM2DBL(RARRAY_PTR(ary)[1]);
            x2 = NUM2DBL(RARRAY_PTR(ary)[2]);
            y2 = NUM2DBL(RARRAY_PTR(ary)[3]);
            volatile VALUE ary_item4 = RARRAY_PTR(ary)[4];
            volatile VALUE ary_item5 = RARRAY_PTR(ary)[5];
            area->type = COLLISION_AREA_LINE;
            area->pointLen = 4;
            if (NIL_P(ary_item4) && NIL_P(ary_item5)) {
                area->points = malloc(sizeof(double) * 4);
                area->points[0] = x; area->points[1] = y;
                area->points[2] = x2; area->points[3] = y2;
            } else {
                double x3 = NUM2DBL(ary_item4), y3 = NUM2DBL(ary_item5);
                if ( ((x == x2) && (y == y2)) || ((x2 == x3) && (y2 == y3)) ) {
                    area->points = malloc(sizeof(double) * 4);
                    area->points[0] = x; area->points[1] = y;
                    area->points[2] = x3; area->points[3] = y3;
                } else if ((x == x3) && (y == y3)) {                    
                    area->points = malloc(sizeof(double) * 4);
                    area->points[0] = x, area->points[1] = y;
                    area->points[2] = x2, area->points[3] = y2;
                } else {
                    area->type = COLLISION_AREA_TRIANGLE;
                    area->pointLen = 6;
                    area->points = malloc(sizeof(double) * len);
                    area->points[0] = x, area->points[1] = y;
                    area->points[2] = x2, area->points[3] = y2;
                    area->points[4] = x3, area->points[5] = y3;
                }
            }
            break;
        default:
            free(area);
            return NULL;
    }
    return area;
}

static VALUE area_initialize(int argc, VALUE argv[], VALUE self) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    collisionArea->type = COLLISION_AREA_NONE;
    collisionArea->next = NULL;
    collisionArea->points = NULL;
    collisionArea->key = NULL;
    collisionArea->active = TRUE;
    volatile VALUE ary_or_hash;
    if (rb_scan_args(argc, argv, "01", &ary_or_hash) == 1) {
        rb_funcall(self, rb_intern("add"), 1, ary_or_hash);
    }
    return Qnil;
}

static VALUE area_add(VALUE self, VALUE area) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea* current = collisionArea;
    while ( current->next ) { current = current->next; }
    if (RB_TYPE_P(area, T_HASH)) {
        volatile VALUE hash_to_a = rb_funcall(area, rb_intern("to_a"), 0);
        int len = RARRAY_LENINT(hash_to_a);
        int i; for (i = 0; i < len; ++i) {
            VALUE assoc = RARRAY_PTR(hash_to_a)[i];
            CollisionArea* newArea = CollisionParseFromArray(RARRAY_PTR(assoc)[1]);
            if (newArea == NULL) { continue; }
            const char *cKey = StringValuePtr(RARRAY_PTR(assoc)[0]);
            int keyLen = strlen(cKey);
            newArea->key = malloc(sizeof(char) * keyLen);
            snprintf(newArea->key, keyLen, cKey);
            current->next = newArea; newArea->prev = current;
            current = current->next;
        }
    } else {
        CollisionArea* newArea = NULL;
        if (RB_TYPE_P(RARRAY_PTR(area)[0], T_FIXNUM)) {
            newArea = CollisionParseFromArray(area);
            if (newArea != NULL) { 
                current->next = newArea; newArea->prev = current;
            }
            return self;
        }
        int len = RARRAY_LENINT(area);
        int i; for (i = 0; i < len; ++i) {
            CollisionArea* newArea = CollisionParseFromArray(RARRAY_PTR(area)[i]);
            if (newArea == NULL) { continue; }
            current->next = newArea; newArea->prev = current;
            current = current->next;
        }
    }
    return self;
}

static VALUE area_add_syntax_sugar(VALUE self, VALUE area) { return area_add(self, area); }

static CollisionArea* SearchArea(CollisionArea* current, VALUE key_or_index) {
    CollisionArea* targetArea = NULL;
    int nowIndex = 0;
    if (RB_TYPE_P(key_or_index, T_FIXNUM)) {
        int targetIndex = NUM2INT(key_or_index) + 1;
        while (current != NULL) {
            if (targetIndex == nowIndex) { targetArea = current; break; }
            nowIndex++;
            current = current->next;
        }
    } else {
        VALUE keyStr = rb_funcall(key_or_index, rb_intern("to_s"), 0);
        const char* keyCStr = StringValuePtr(keyStr);
        int keyLen = strlen(keyCStr);
        while (current != NULL) {
            if (strncmp(keyCStr, current->key, keyLen) == 0) {
                targetArea = current; break; 
            }
            current = current->next;
        }         
    }
    return targetArea;
}

static VALUE area_delete(VALUE self, VALUE key_or_index) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea* deleteArea = SearchArea(collisionArea, key_or_index);

    if (deleteArea != NULL) {
        deleteArea->prev = deleteArea->next;
        free(deleteArea->key);
        free(deleteArea->points);
        free(deleteArea);
    }
    return self;
}

static VALUE PointsToRArray(CollisionArea *area) {
    int i; volatile VALUE result = Qundef;
    switch (area->type) {
        case COLLISION_AREA_LINE:
            result = rb_ary_new2(6);
            for (i = 0; i < 6; ++i) rb_ary_push(result, rb_int_new(area->points[i]));
            rb_ary_push(result, Qnil); rb_ary_push(result, Qnil);
            break;
        case COLLISION_AREA_RECT:
            result = rb_ary_new2(4);
            rb_ary_push(result, rb_int_new(area->points[0])); rb_ary_push(result, rb_int_new(area->points[1]));
            rb_ary_push(result, rb_int_new(area->points[4])); rb_ary_push(result, rb_int_new(area->points[5]));
            break;
        default:
            result = rb_ary_new2(area->pointLen);
            for (i = 0; i < area->pointLen; ++i) rb_ary_push(result, rb_int_new(area->points[i]));
    }
    return result;
}

static VALUE RealPointsToRArray(int x, int y, CollisionArea *area) {
    int i; volatile VALUE result = Qundef;
    switch (area->type) {
        case COLLISION_AREA_LINE:
            result = rb_ary_new2(4);
            for (i = 0; i < 4; ++i) {
                rb_ary_push(result, rb_int_new(area->points[i] + (i % 2 == 0 ? x : y)));
            }
            rb_ary_push(result, Qnil); rb_ary_push(result, Qnil);
            break;
        case COLLISION_AREA_RECT:
            result = rb_ary_new2(4);
            rb_ary_push(result, rb_int_new(area->points[0]+x)); rb_ary_push(result, rb_int_new(area->points[1]+y));
            rb_ary_push(result, rb_int_new(area->points[4]+x)); rb_ary_push(result, rb_int_new(area->points[5]+y));
            break;
        default:
            result = rb_ary_new2(area->pointLen);
            for (i = 0; i < area->pointLen; ++i) {
                rb_ary_push(result, rb_int_new(area->points[i] + (i % 2 == 0 ? x : y)));
            }
    }
    return result;
}


static VALUE area_get(VALUE self, VALUE key_or_index) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea* targetArea = SearchArea(collisionArea, key_or_index);
    if (!targetArea) return Qnil;
    volatile VALUE ary = PointsToRArray(targetArea);
    if (targetArea->key != NULL) {
        volatile VALUE result = rb_hash_new();
        rb_hash_aset(result, ID2SYM(rb_intern(targetArea->key)), ary);
        return result; 
    } else {
        return ary;
    }
}
static VALUE  area_insert(VALUE self, VALUE key_or_index, VALUE area) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    
    CollisionArea* updateArea = CollisionParseFromArray(area);
    CollisionArea* targetArea = SearchArea(collisionArea, key_or_index);
    if (targetArea) {
        updateArea->prev = targetArea->prev;
        updateArea->next = targetArea->next;
        updateArea->key = targetArea->key;
        free(targetArea->points);
        free(targetArea);
    } else {
        if (!RB_TYPE_P(key_or_index, T_FIXNUM)) {
            const char* cKey = updateArea->key = StringValuePtr(key_or_index);
            int keyLen = strlen(cKey);
            updateArea->key = malloc(sizeof(char) * keyLen);
            snprintf(updateArea->key, keyLen, cKey);     
        }
        CollisionArea* current = collisionArea;
        while ( current->next ) { current = current->next; }       
        current->next = updateArea;
    }
    return area_add(self, area);
}

static VALUE area_clear(VALUE self) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea* current = collisionArea->next;
    while(current) {
        free(current->points);
        if (current->key != NULL) { free(current->key); }
        CollisionArea* old = current; current = current->next; free(old);
    }
    collisionArea->next = NULL;
    return self;
}
static VALUE area_set(VALUE self, VALUE area) { area_clear(self); area_add(self, area); return self; }


static VALUE area_active(VALUE self, VALUE key_or_index) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea* targetArea = SearchArea(collisionArea, key_or_index);
    if (!targetArea) return Qnil;
    targetArea->active = TRUE;
    return Qnil;
}

static VALUE area_deactive(VALUE self, VALUE key_or_index) {
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea* targetArea = SearchArea(collisionArea, key_or_index);
    if (!targetArea) return Qnil;
    targetArea->active = FALSE;
    return Qnil;
}

static VALUE area_inspect(VALUE self) {
    volatile VALUE array = rb_ary_new();
    CollisionArea *collisionArea; Data_Get_Struct(self, CollisionArea, collisionArea);
    CollisionArea * buff = collisionArea;
    while (buff->next) {
        volatile VALUE child = PointsToRArray(buff->next);
        if (buff->next->key != NULL) {
            volatile VALUE hash = rb_hash_new();
            rb_hash_aset(hash, ID2SYM(rb_intern(buff->next->key)), child);
            rb_ary_push(array, hash);
        } else {
            rb_ary_push(array, child);
        }
        buff = buff->next;
    }
    return rb_ary_to_s(array);
}

static void ConvertRealHitArea(CollisionArea * area, double x, double y, double resultPoints[]) {
    resultPoints[0] = area->points[0] + x; resultPoints[1] = area->points[1] + y;
    switch (area->type) {
        case COLLISION_AREA_CIRCLE:
            resultPoints[2] = area->points[2];
            break;
        case COLLISION_AREA_LINE:
            resultPoints[2] = area->points[2] + x; resultPoints[3] = area->points[3] + y;
            break;
        case COLLISION_AREA_TRIANGLE:
            resultPoints[2] = area->points[2] + x; resultPoints[3] = area->points[3] + y;
            resultPoints[4] = area->points[4] + x; resultPoints[5] = area->points[5] + y;
            break;
        case COLLISION_AREA_RECT:
            resultPoints[2] = area->points[2] + x; resultPoints[3] = area->points[3] + y;
            resultPoints[4] = area->points[4] + x; resultPoints[5] = area->points[5] + y;
            resultPoints[6] = area->points[6] + x; resultPoints[7] = area->points[7] + y;
            break;
        default: break;
    }
}

static inline double* GetCollisionTypeAndCArray(VALUE rb_ary, int* type, int *arrayLen) {
    int i, len = RARRAY_LENINT(rb_ary);
    double *array = NULL;
    double x, y, x2, y2;
    switch (len) {
        case 2:
            *type = COLLISION_AREA_POINT;
            *arrayLen = 2;
            array = malloc(sizeof(double) * len);
            array[0] = NUM2DBL(RARRAY_PTR(rb_ary)[0]);
            array[1] = NUM2DBL(RARRAY_PTR(rb_ary)[1]);

            break;
        case 3:
            *type = COLLISION_AREA_CIRCLE;
            *arrayLen = 3;
            array = malloc(sizeof(double) * len);
            for (i = 0; i < len; ++i) { array[i] = NUM2DBL(RARRAY_PTR(rb_ary)[i]); }
            break;
        case 4:
            *type = COLLISION_AREA_RECT;
            *arrayLen = 8;
            array = malloc(sizeof(double) * (*arrayLen));
            array[0] = NUM2DBL(RARRAY_PTR(rb_ary)[0]); 
            array[1] = NUM2DBL(RARRAY_PTR(rb_ary)[1]); 
            array[2] = NUM2DBL(RARRAY_PTR(rb_ary)[2]); 
            array[3] = array[1]; 
            array[4] = array[2];
            array[5] = NUM2DBL(RARRAY_PTR(rb_ary)[3]); 
            array[6] = array[0];
            array[7] = array[5];
            break;
        case 6:
            x = NUM2DBL(RARRAY_PTR(rb_ary)[0]);
            y = NUM2DBL(RARRAY_PTR(rb_ary)[1]);
            x2 = NUM2DBL(RARRAY_PTR(rb_ary)[2]);
            y2 = NUM2DBL(RARRAY_PTR(rb_ary)[3]);
            volatile VALUE ary_item4 = RARRAY_PTR(rb_ary)[4];
            volatile VALUE ary_item5 = RARRAY_PTR(rb_ary)[5];
            *type = COLLISION_AREA_LINE;
            *arrayLen = 4;
            if (NIL_P(ary_item4) && NIL_P(ary_item5)) {
                array = malloc(sizeof(double) * 4);
                array[0] = x; array[1] = y;
                array[2] = x2; array[3] = y2;
            } else {
                double x3 = NUM2DBL(ary_item4), y3 = NUM2DBL(ary_item5);
                if ( ((x == x2) && (y == y2)) || ((x2 == x3) && (y2 == y3)) ) {
                    array = malloc(sizeof(double) * 4);
                    array[0] = x; array[1] = y;
                    array[2] = x3; array[3] = y3;
                } else if ((x == x3) && (y == y3)) {                    
                    array = malloc(sizeof(double) * 4);
                    array[0] = x, array[1] = y;
                    array[2] = x2, array[3] = y2;
                } else {
                    *type = COLLISION_AREA_TRIANGLE;
                    *arrayLen = 6;
                    array = malloc(sizeof(double) * len);
                    array[0] = x, array[1] = y;
                    array[2] = x2, array[3] = y2;
                    array[4] = x3, array[5] = y3;
                }
            }
            break;
        default:
            P("Can't Convert Array!");
            break;
    }
    return array;
}

static inline BOOL Hit(double *aPoints, int aPointsLen, double *bPoints, int bPointsLen, int collisionType, HitData *outHitData) {
            BOOL hitFlag = FALSE;
            switch ( collisionType) {
                case (COLLISION_AREA_POINT | COLLISION_AREA_POINT):
                    hitFlag = HitPointAndPoint(aPoints, bPoints, outHitData);
                    break;
                case (COLLISION_AREA_POINT | COLLISION_AREA_LINE):
                    if (aPointsLen == 2) {
                        hitFlag = HitPointAndLine(aPoints, bPoints, outHitData);
                    } else {
                        hitFlag = HitPointAndLine(bPoints, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_POINT | COLLISION_AREA_CIRCLE):
                    if (aPointsLen == 2) {
                        hitFlag = HitPointAndCircle(aPoints, bPoints, outHitData);
                    } else {
                        hitFlag = HitPointAndCircle(bPoints, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_POINT | COLLISION_AREA_TRIANGLE):
                    if (aPointsLen == 2) {
                        hitFlag = HitPointAndPolygon(aPoints, bPointsLen, bPoints, outHitData);
                    } else {
                        hitFlag = HitPointAndPolygon(bPoints, aPointsLen, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_POINT | COLLISION_AREA_RECT):
                    if (aPointsLen == 2) {
                        hitFlag = HitPointAndRect(aPoints, bPoints, outHitData);
                    } else {
                        hitFlag = HitPointAndRect(bPoints, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_LINE | COLLISION_AREA_RECT):
                    if (aPointsLen == 4) {
                        hitFlag = HitLineAndPolygon(aPoints, bPointsLen, bPoints, outHitData);
                    } else {
                        hitFlag = HitLineAndPolygon(bPoints, aPointsLen, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_RECT | COLLISION_AREA_RECT):
                    hitFlag = HitRectAndRect(aPoints, bPoints, outHitData);
                    break;
                case (COLLISION_AREA_RECT | COLLISION_AREA_TRIANGLE):
                    hitFlag = HitPolygonAndPolygon(aPointsLen,  aPoints, bPointsLen, bPoints, outHitData);
                    break;
                case (COLLISION_AREA_CIRCLE | COLLISION_AREA_CIRCLE):
                    hitFlag = HitCircleAndCircle(aPoints, bPoints, outHitData);
                    break;
                case (COLLISION_AREA_CIRCLE | COLLISION_AREA_RECT):
                    if (aPointsLen == 3) {
                        hitFlag = HitCircleAndPolygon(aPoints, bPointsLen, bPoints, outHitData);
                    } else {
                        hitFlag = HitCircleAndPolygon(bPoints, aPointsLen, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_CIRCLE | COLLISION_AREA_LINE):
                    if (aPointsLen == 3) {
                        hitFlag = HitCircleAndLine(aPoints, bPoints, outHitData);
                    } else {
                        hitFlag = HitCircleAndLine(bPoints, aPoints, outHitData);
                    }
                    break;
                case (COLLISION_AREA_LINE | COLLISION_AREA_LINE):
                    hitFlag = HitLineAndLine(bPoints, aPoints, outHitData);
                    break;
                default:
                    P("error!");
                    break;
            }

    return hitFlag;
}

static inline HitObjectData* HitActor(VALUE obj_a, VALUE obj_b) { 
    double ax = NUM2DBL(rb_funcall(obj_a, rb_intern("x"), 0)), ay = NUM2DBL(rb_funcall(obj_a, rb_intern("y"), 0));
    double bx = NUM2DBL(rb_funcall(obj_b, rb_intern("x"), 0)), by = NUM2DBL(rb_funcall(obj_b, rb_intern("y"), 0));
    CollisionArea* aArea; Data_Get_Struct(rb_ivar_get(obj_a, rb_intern("@hit_area")), CollisionArea, aArea);
    CollisionArea* bArea; Data_Get_Struct(rb_ivar_get(obj_b, rb_intern("@hit_area")), CollisionArea, bArea);
    double aPoints[10] = { 0 }, bPoints[10] = { 0 };
    CollisionArea *aAreaBuff = aArea->next, *bAreaBuff = bArea->next;
    HitObjectData *headHitObjectData = malloc(sizeof(HitObjectData));
    headHitObjectData->next = NULL;
    HitObjectData* currentHitObjectData = headHitObjectData;
    int aIndex = 0;
    while (aAreaBuff) {
        if (!aAreaBuff->active) { continue; }
        ConvertRealHitArea(aAreaBuff, ax, ay, aPoints);
        int bIndex = 0;
        while (bAreaBuff) {
            if (!bAreaBuff->active) { continue; }
            ConvertRealHitArea(bAreaBuff, bx, by, bPoints);
            HitData hitData;
            if (Hit(aPoints, aAreaBuff->pointLen, bPoints, bAreaBuff->pointLen, aAreaBuff->type | bAreaBuff->type, &hitData)) { 
                AddNextHitObjectData(currentHitObjectData, aArea, aIndex, bArea, bIndex, &hitData); 
            }
            bAreaBuff = bAreaBuff->next;
            bIndex++;
        }
        aAreaBuff = aAreaBuff->next;
        aIndex++;
    }

    if (headHitObjectData->next != NULL) {
        return headHitObjectData; 
    } else {
        free(headHitObjectData);
        return NULL;
    }
}

static inline VALUE OneSideHitObjectDataToRB(HitObjectData * headHitObjectData, VALUE obj_a, VALUE obj_b) {
    volatile VALUE result = rb_hash_new();
    HitObjectData* currentHitObjectData = headHitObjectData->next;

    while (currentHitObjectData) {
        volatile VALUE self_key = rb_int_new(currentHitObjectData->selfIndex);
        volatile VALUE target_key = rb_int_new(currentHitObjectData->targetIndex);
        volatile VALUE self_info = Qundef; 

        if (currentHitObjectData->selfArea->key) { self_key = ID2SYM(rb_intern(currentHitObjectData->selfArea->key)); }
        if (currentHitObjectData->targetArea->key) { target_key = ID2SYM(rb_intern(currentHitObjectData->selfArea->key)); }

        self_info = rb_hash_lookup(result, self_key);
        if (NIL_P(self_info)) {
            self_info = rb_hash_new();
            rb_hash_aset(result, self_key, self_info);
        } else {
        }
        rb_hash_aset(self_info, target_key, rb_hash_new());
        HitObjectData *next = currentHitObjectData->next;
        free(currentHitObjectData); currentHitObjectData = next;
    }
    return result;
}

static VALUE HitObjectDataToRB(HitObjectData * headHitObjectData, VALUE obj_a, VALUE obj_b) {
    volatile VALUE self_hit_data = rb_hash_new(); 
    volatile VALUE target_hit_data =  rb_hash_new();
    volatile VALUE result = rb_assoc_new(self_hit_data, target_hit_data);

    HitObjectData* currentHitObjectData = headHitObjectData->next;

    while (currentHitObjectData) {
        volatile VALUE self_key = rb_int_new(currentHitObjectData->selfIndex);
        volatile VALUE target_key = rb_int_new(currentHitObjectData->targetIndex);
        volatile VALUE self_info = Qundef, target_info = Qundef;

        if (currentHitObjectData->selfArea->key) { self_key = ID2SYM(rb_intern(currentHitObjectData->selfArea->key)); }
        if (currentHitObjectData->targetArea->key) { target_key = ID2SYM(rb_intern(currentHitObjectData->selfArea->key)); }

        self_info = rb_hash_lookup(self_hit_data , self_key);
        target_info = rb_hash_lookup(target_hit_data , target_key);
        

        if (NIL_P(self_info)) {
            self_info = rb_hash_new();
            rb_hash_aset(self_hit_data, self_key, self_info);
        } else {
        }

        if (NIL_P(target_info)) {
            target_info = rb_hash_new(); 
            rb_hash_aset(target_hit_data, target_key, target_info);
        } else {
        }

        rb_hash_aset(self_info, target_key, rb_hash_new());
        rb_hash_aset(target_info, self_key, rb_hash_new());

        HitObjectData *next = currentHitObjectData->next;
        free(currentHitObjectData); currentHitObjectData = next;
    }
    return result;
}

static inline VALUE CollisionHitActorImpl(VALUE obj_a, VALUE obj_b, BOOL block_flag) {
    HitObjectData *headHitObjectData = HitActor(obj_a, obj_b);
    if (headHitObjectData) {
        volatile VALUE result = HitObjectDataToRB(headHitObjectData, obj_a, obj_b);
        if (block_flag && rb_block_given_p()) { rb_yield(result); return Qnil; } 
        return result;
    }
    return Qnil;
}

static VALUE collision_module_hit(int argc, VALUE argv[], VALUE self ) {
    VALUE obj_a, obj_b, block;
    rb_scan_args(argc, argv, "2&", &obj_a, &obj_b, &block);
    volatile VALUE obj_a_ary = rb_funcall(obj_a, rb_intern("to_a"), 0);
    volatile VALUE obj_b_ary = rb_funcall(obj_b, rb_intern("to_a"), 0);

    if (!RB_TYPE_P(RARRAY_PTR(obj_a_ary)[0], T_ARRAY)) obj_a_ary = rb_ary_new3(1, obj_a_ary);
    if (!RB_TYPE_P(RARRAY_PTR(obj_b_ary)[0], T_ARRAY)) obj_b_ary = rb_ary_new3(1, obj_b_ary);

    int objALen = RARRAY_LENINT(obj_a_ary);
    int objBLen = RARRAY_LENINT(obj_b_ary);
    int i,j;
    int aType = -1, bType = -1, aArrayLen = 0, bArrayLen = 0; 
    double *aArray = NULL, *bArray = NULL;
    HitData hitData;
    BOOL hitFlag = FALSE;
    for (i = 0; i < objALen; ++i) {
            aArray = GetCollisionTypeAndCArray(RARRAY_PTR(obj_a_ary)[i], &aType, &aArrayLen);
            p("\n aArray[0] = %lf aArray[1] = %lf \n", aArray[0], aArray[1]); 

        for (j = 0; j < objBLen; ++j) {
            bArray = GetCollisionTypeAndCArray(RARRAY_PTR(obj_b_ary)[j], &bType, &bArrayLen);
            if (Hit(aArray, aArrayLen, bArray, bArrayLen, aType | bType, &hitData)) {
                hitFlag = TRUE; break;
            }
            free(bArray);
        }
        free(aArray);
    }

    return hitFlag ? Qtrue : Qfalse;
}

static VALUE collision_module_hit_actor(int argc, VALUE argv[], VALUE self ) {
    VALUE obj_a, obj_b, block;
    rb_scan_args(argc, argv, "2&", &obj_a, &obj_b, &block);
    const VALUE scene_class = rb_path2class("SDLUdon::Scene");
    BOOL obj_a_is_scene = (rb_obj_is_kind_of(obj_a, scene_class) == Qtrue);
    BOOL obj_b_is_scene = (rb_obj_is_kind_of(obj_b, scene_class) == Qtrue);

    if (obj_a_is_scene || obj_b_is_scene) {
        volatile VALUE self_result = rb_hash_new(), target_result = rb_hash_new();
        volatile VALUE result = rb_assoc_new(self_result, target_result);
        
        int i, j; BOOL hitFlag = FALSE;
        if (obj_a_is_scene && obj_b_is_scene) {
            VALUE self_pool = rb_ivar_get(obj_a, rb_intern("@pool"));
            int selfLen = RARRAY_LENINT(self_pool);
            VALUE target_pool = rb_ivar_get(obj_b, rb_intern("@pool"));
            int targetLen = RARRAY_LENINT(target_pool);
            for (i = 0; i < selfLen; ++i) {
                for (j = 0; j < targetLen; ++j) {
                    VALUE a = RARRAY_PTR(self_pool)[i], b = RARRAY_PTR(target_pool)[j];
                    volatile VALUE hit_data = CollisionHitActorImpl(a, b, FALSE);
                    if (!NIL_P(hit_data)) {
                        hitFlag = TRUE;
                        VALUE child_info = rb_hash_lookup(self_result, a);
                        if (NIL_P(child_info)) { child_info = rb_hash_aset(self_result, a, rb_hash_new()); }
                        rb_hash_aset(child_info, b, RARRAY_PTR(hit_data)[0]);

                        VALUE target_child_info = rb_hash_lookup(target_result, b);
                        if (NIL_P(target_child_info)) { target_child_info = rb_hash_aset(target_result, b, rb_hash_new()); }
                        rb_hash_aset(target_child_info, a, RARRAY_PTR(hit_data)[1]);
                    }
                }
            }
        } else {
            if (!obj_a_is_scene) { volatile VALUE temp = obj_a; obj_a = obj_b; obj_b = temp; }
            VALUE self_pool = rb_ivar_get(obj_a, rb_intern("@pool"));
            int selfLen = RARRAY_LENINT(self_pool);

            for (i = 0; i < selfLen; ++i) {
                VALUE self_child = RARRAY_PTR(self_pool)[i];
                volatile VALUE hit_data = CollisionHitActorImpl(self_child, obj_b, FALSE);
                if (!NIL_P(hit_data)) {
                    hitFlag  = TRUE;
                    VALUE child_info = rb_hash_lookup(self_result, self_child);
                    if (NIL_P(child_info)) { child_info = rb_hash_aset(self_result, self_child, rb_hash_new()); }
                    rb_hash_aset(child_info, obj_b, RARRAY_PTR(hit_data)[0]);
                    rb_hash_aset(target_result, self_child, RARRAY_PTR(hit_data)[1]);
                }
            }
            
            if (!obj_a_is_scene) { result = rb_assoc_new(target_result, self_result); }
        }

        if (hitFlag) {
            if (rb_block_given_p()) { rb_yield(result) ; return Qnil; }
            return result;
        }
        return Qnil;
    }
    return CollisionHitActorImpl(obj_a, obj_b, TRUE);
}
static VALUE collision_hit(int argc, VALUE argv[], VALUE self) {
    VALUE target; rb_scan_args(argc, argv, "1", &target);
    HitObjectData *headHitObjectData = HitActor(self, target);
    if (headHitObjectData) { return OneSideHitObjectDataToRB(headHitObjectData, self, target); }
    return Qnil;
}


static VALUE collision_get_real_hit_area(VALUE self) {
    int x = NUM2INT(rb_funcall(self, rb_intern("x"), 0));
    int y = NUM2INT(rb_funcall(self, rb_intern("y"), 0));
    VALUE hit_area = rb_ivar_get(self, rb_intern("@hit_area"));
    CollisionArea * areaData; Data_Get_Struct(hit_area, CollisionArea, areaData);
    volatile VALUE result_ary = rb_ary_new();

    CollisionArea *current = areaData->next;

    while (current) {
        volatile VALUE ary = RealPointsToRArray(x, y, current);
        if (current->key != NULL) {
            volatile VALUE hash = rb_hash_new();
            rb_hash_aset(hash, ID2SYM(rb_intern(current->key)), ary);
            rb_ary_push(result_ary, hash);
        } else {
            rb_ary_push(result_ary, ary);
        }
        current = current->next;
    }
    return RARRAY_LENINT(result_ary) == 1 ? RARRAY_PTR(result_ary)[0] : result_ary;
}
void Init_collision(VALUE parent_module) {
    VALUE module_collision = rb_define_module_under(parent_module, "Collision");
    rb_define_module_function(module_collision, "hit", collision_module_hit, -1);
    rb_define_module_function(module_collision, "hit_actor", collision_module_hit_actor , -1);
    rb_define_method(module_collision, "init_collision", collision_init_collision , -1);
    rb_define_method(module_collision, "hit", collision_hit, -1);
    rb_define_method(module_collision, "real_hit_area", collision_get_real_hit_area, 0);
    rb_define_method(module_collision, "hit_area", collision_get_hit_area, 0);
    rb_define_method(module_collision, "collidable=", collision_collidable, 1);
    rb_define_method(module_collision, "collidable?", collision_is_collied, 0);

    VALUE area = rb_define_class_under(module_collision, "Area", rb_cObject); 
    rb_define_alloc_func(area, area_alloc);
    rb_define_private_method(area, "initialize", area_initialize, -1);
    rb_define_method(area, "add", area_add, 1); 
    rb_define_method(area, "set", area_set, 1); 
    rb_define_method(area, "<<", area_add_syntax_sugar, 1); 
    rb_define_method(area, "delete", area_delete, 1);
    rb_define_method(area, "clear", area_clear, 0);
    rb_define_method(area, "[]", area_get, 1); 
    rb_define_method(area, "[]=", area_insert, 2);
    rb_define_method(area, "active", area_active, 1);
    rb_define_method(area, "deactive", area_deactive, 1);
    rb_define_method(area, "inspect", area_inspect, 0);
}