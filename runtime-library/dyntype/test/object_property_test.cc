/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include <gtest/gtest.h>

class ObjectPropertyTest : public testing::Test {
  protected:
    virtual void SetUp() {
        ctx = dyntype_context_init();
    }

    virtual void TearDown() {
        dyntype_context_destroy(ctx);
    }

    dyn_ctx_t ctx;
};

TEST_F(ObjectPropertyTest, object_set_and_has_and_get_property) {
    dyn_value_t obj = dyntype_new_object(ctx);

    int ext_data = 1000;

    dyn_value_t num = dyntype_new_number(ctx, 2147483649);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "string");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref = dyntype_new_extref(ctx, (void *)(uintptr_t)ext_data,
                                            external_ref_tag::ExtObj, NULL);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop1", num), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop2", boolean), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop3", undefined), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop4", null), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop5", str), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop6", array), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop7", extref), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop8", obj1), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), 0);

    dyn_value_t num_v = dyntype_get_property(ctx, obj, "prop1");
    double v = 0;
    dyntype_to_number(ctx, num_v, &v);
    EXPECT_EQ(v, 2147483649);
    dyntype_release(ctx, num_v);

    dyn_value_t boolean_v = dyntype_get_property(ctx, obj, "prop2");
    bool v1 = false;
    dyntype_to_bool(ctx, boolean_v, &v1);
    EXPECT_EQ(v1, true);

    dyn_value_t undefined_v = dyntype_get_property(ctx, obj, "prop3");
    EXPECT_TRUE(dyntype_is_undefined(ctx, undefined_v));

    dyn_value_t null_v = dyntype_get_property(ctx, obj, "prop4");
    EXPECT_TRUE(dyntype_is_null(ctx, null_v));

    dyn_value_t str_v = dyntype_get_property(ctx, obj, "prop5");
    char const *target = "string";
    char *v2 = nullptr;
    dyntype_to_cstring(ctx, str_v, &v2);
    EXPECT_STREQ(v2, target);
    dyntype_release(ctx, str_v);

    dyn_value_t array_v = dyntype_get_property(ctx, obj, "prop6");
    EXPECT_TRUE(dyntype_is_array(ctx, array_v));
    dyntype_release(ctx, array_v);

    dyn_value_t extref_v = dyntype_get_property(ctx, obj, "prop7");
    EXPECT_TRUE(dyntype_is_extref(ctx, extref_v));

    dyn_value_t obj1_v = dyntype_get_property(ctx, obj, "prop8");
    EXPECT_TRUE(dyntype_is_object(ctx, obj1_v));
    dyntype_release(ctx, obj1_v);


    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), 0);

    dyntype_dump_value(ctx, obj);
    char *buffer = new char[1024 * 10];
    dyntype_dump_value_buffer(ctx, obj, buffer, 1024 * 10);

    delete[] buffer;

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), ExtObj);
    EXPECT_EQ((int)(uintptr_t)extref_prop, 1000);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop7"), 1);
    dyntype_release(ctx, extref);
    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
}

TEST_F(ObjectPropertyTest, object_define_and_has_and_get_property) {
    dyn_value_t obj = dyntype_new_object(ctx);
    int ext_data = 1000;

    dyn_value_t num = dyntype_new_number(ctx, -10.1);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "  ");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref = dyntype_new_extref(ctx, (void *)(uintptr_t)ext_data,
                                            external_ref_tag::ExtObj, NULL);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    dyn_value_t desc1 = dyntype_new_object(ctx);
    dyn_value_t desc1_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc1, "configurable", desc1_v);
    dyntype_set_property(ctx, desc1, "value", num);

    dyn_value_t desc2 = dyntype_new_object(ctx);
    dyn_value_t desc2_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc2, "writable", desc2_v);
    dyntype_set_property(ctx, desc2, "value", boolean);

    dyn_value_t desc3 = dyntype_new_object(ctx);
    dyn_value_t desc3_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc3, "enumerable", desc3_v);
    dyntype_set_property(ctx, desc3, "value", undefined);

    dyn_value_t desc4 = dyntype_new_object(ctx);
    dyn_value_t desc4_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc4, "configurable", desc4_v);
    dyntype_set_property(ctx, desc4, "value", null);

    dyn_value_t desc5 = dyntype_new_object(ctx);
    dyn_value_t desc5_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc5, "writable", desc5_v);
    dyntype_set_property(ctx, desc5, "value", str);

    dyn_value_t desc6 = dyntype_new_object(ctx);
    dyn_value_t desc6_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc6, "enumerable", desc6_v);
    dyntype_set_property(ctx, desc6, "value", array);

    dyn_value_t desc7 = dyntype_new_object(ctx);
    dyn_value_t desc7_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc7, "configurable", desc7_v);
    dyntype_set_property(ctx, desc7, "value", extref);

    dyn_value_t desc8 = dyntype_new_object(ctx);
    dyn_value_t desc8_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc8, "writable", desc8_v);
    dyntype_set_property(ctx, desc8, "value", obj1);

    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop1", desc1), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop2", desc2), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop3", desc3), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop4", desc4), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop5", desc5), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop6", desc6), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop7", desc7), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop8", desc8), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), 1);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), 0);

    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop not a object", dyntype_new_boolean(ctx, false)),
              -DYNTYPE_TYPEERR);

    dyn_value_t num_v = dyntype_get_property(ctx, obj, "prop1");
    double v = 0;
    dyntype_to_number(ctx, num_v, &v);
    EXPECT_EQ(v, -10.1);
    dyntype_release(ctx, num_v);

    dyn_value_t boolean_v = dyntype_get_property(ctx, obj, "prop2");
    bool v1 = false;
    dyntype_to_bool(ctx, boolean_v, &v1);
    EXPECT_EQ(v1, true);

    dyn_value_t undefined_v = dyntype_get_property(ctx, obj, "prop3");
    EXPECT_TRUE(dyntype_is_undefined(ctx, undefined_v));

    dyn_value_t null_v = dyntype_get_property(ctx, obj, "prop4");
    EXPECT_TRUE(dyntype_is_null(ctx, null_v));

    dyn_value_t str_v = dyntype_get_property(ctx, obj, "prop5");
    char const *target = "  ";
    char *v2 = nullptr;
    dyntype_to_cstring(ctx, str_v, &v2);
    EXPECT_STREQ(v2, target);
    dyntype_release(ctx, str_v);

    dyn_value_t array_v = dyntype_get_property(ctx, obj, "prop6");
    EXPECT_TRUE(dyntype_is_array(ctx, array_v));
    dyntype_release(ctx, array_v);

    dyn_value_t extref_v = dyntype_get_property(ctx, obj, "prop7");
    EXPECT_TRUE(dyntype_is_extref(ctx, extref_v));

    dyn_value_t obj1_v = dyntype_get_property(ctx, obj, "prop8");
    EXPECT_TRUE(dyntype_is_object(ctx, obj1_v));
    dyntype_release(ctx, obj1_v);

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), DYNTYPE_SUCCESS);
    EXPECT_EQ((int)(uintptr_t)extref_prop, 1000);

    dyntype_release(ctx, extref);
    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);

    dyntype_release(ctx, desc1);
    dyntype_release(ctx, desc2);
    dyntype_release(ctx, desc3);
    dyntype_release(ctx, desc4);
    dyntype_release(ctx, desc5);
    dyntype_release(ctx, desc6);
    dyntype_release(ctx, desc7);
    dyntype_release(ctx, desc8);
}

TEST_F(ObjectPropertyTest, object_set_and_delete_property) {
   dyn_value_t obj = dyntype_new_object(ctx);

    int ext_data = 1000;

    dyn_value_t num = dyntype_new_number(ctx, 2147483649);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "string");
    dyn_value_t array = dyntype_new_array(ctx);
    EXPECT_TRUE(dyntype_is_array(ctx, array));
    dyn_value_t extref = dyntype_new_extref(ctx, (void *)(uintptr_t)ext_data,
                                            external_ref_tag::ExtObj, NULL);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop1", num), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop2", boolean), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop3", undefined), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop4", null), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop5", str), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop6", array), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop7", extref), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_set_property(ctx, obj, "prop8", obj1), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop1"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop2"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop3"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop4"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop5"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop6"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop7"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop8"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop9"), DYNTYPE_FALSE);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop9"), DYNTYPE_FALSE);

    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
}

TEST_F(ObjectPropertyTest, object_define_and_delete_property) {
    dyn_value_t obj = dyntype_new_object(ctx);
    int ext_data = 1000;

    dyn_value_t num = dyntype_new_number(ctx, -10.1);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t undefined = dyntype_new_undefined(ctx);
    dyn_value_t null = dyntype_new_null(ctx);
    dyn_value_t str = dyntype_new_string(ctx, "  ");
    dyn_value_t array = dyntype_new_array(ctx);
    dyn_value_t extref = dyntype_new_extref(ctx, (void *)(uintptr_t)ext_data,
                                            external_ref_tag::ExtObj, NULL);
    dyn_value_t obj1 = dyntype_new_object(ctx);

    dyn_value_t desc1 = dyntype_new_object(ctx);
    dyn_value_t desc1_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc1, "configurable", desc1_v);
    dyntype_set_property(ctx, desc1, "value", num);

    dyn_value_t desc2 = dyntype_new_object(ctx);
    dyn_value_t desc2_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc2, "writable", desc2_v);
    dyntype_set_property(ctx, desc2, "value", boolean);

    dyn_value_t desc3 = dyntype_new_object(ctx);
    dyn_value_t desc3_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc3, "enumerable", desc3_v);
    dyntype_set_property(ctx, desc3, "value", undefined);

    dyn_value_t desc4 = dyntype_new_object(ctx);
    dyn_value_t desc4_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc4, "configurable", desc4_v);
    dyntype_set_property(ctx, desc4, "value", null);

    dyn_value_t desc5 = dyntype_new_object(ctx);
    dyn_value_t desc5_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc5, "writable", desc5_v);
    dyntype_set_property(ctx, desc5, "value", str);

    dyn_value_t desc6 = dyntype_new_object(ctx);
    dyn_value_t desc6_v = dyntype_new_boolean(ctx, false);
    dyntype_set_property(ctx, desc6, "enumerable", desc6_v);
    dyntype_set_property(ctx, desc6, "value", array);

    dyn_value_t desc7 = dyntype_new_object(ctx);
    dyn_value_t desc7_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc7, "configurable", desc7_v);
    dyntype_set_property(ctx, desc7, "value", extref);

    dyn_value_t desc8 = dyntype_new_object(ctx);
    dyn_value_t desc8_v = dyntype_new_boolean(ctx, true);
    dyntype_set_property(ctx, desc8, "writable", desc8_v);
    dyntype_set_property(ctx, desc8, "value", obj1);

    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop1", desc1), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop2", desc2), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop3", desc3), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop4", desc4), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop5", desc5), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop6", desc6), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop7", desc7), DYNTYPE_SUCCESS);
    EXPECT_EQ(dyntype_define_property(ctx, obj, "prop8", desc8), DYNTYPE_SUCCESS);

    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop1"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop2"), DYNTYPE_FALSE); // writable
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop3"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop4"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop5"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop6"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop7"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_delete_property(ctx, obj, "prop8"), DYNTYPE_FALSE);

    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop1"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop2"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop3"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop4"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop5"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop6"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop7"), DYNTYPE_FALSE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "prop8"), DYNTYPE_TRUE);

    void *extref_prop = nullptr;
    EXPECT_EQ(dyntype_to_extref(ctx, extref, &extref_prop), DYNTYPE_SUCCESS);
    EXPECT_EQ((int)(uintptr_t)extref_prop, 1000);


    dyntype_release(ctx, obj);
    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);

    dyntype_release(ctx, desc1);
    dyntype_release(ctx, desc2);
    dyntype_release(ctx, desc3);
    dyntype_release(ctx, desc4);
    dyntype_release(ctx, desc5);
    dyntype_release(ctx, desc6);
    dyntype_release(ctx, desc7);
    dyntype_release(ctx, desc8);
}

TEST_F(ObjectPropertyTest, map_function_test) {
    dyn_value_t obj = dyntype_new_object_with_class(ctx, "Map", 0, NULL);

    dyn_value_t num = dyntype_new_number(ctx, -10.1);
    dyn_value_t boolean = dyntype_new_boolean(ctx, true);
    dyn_value_t str = dyntype_new_string(ctx, "123");
    dyn_value_t array = dyntype_new_array(ctx);

    dyn_value_t argv[10];
    EXPECT_EQ(dyntype_has_property(ctx, obj, "set"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "get"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "has"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "delete"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "clear"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "size"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "forEach"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "values"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "keys"), DYNTYPE_TRUE);
    EXPECT_EQ(dyntype_has_property(ctx, obj, "entries"), DYNTYPE_TRUE);

    argv[0] = num;
    argv[1] = boolean;
    argv[2] = str;
    argv[3] = array;
    dyn_value_t ret;
    ret = dyntype_invoke(ctx, "set", obj, 2, argv); // set  num -> boolean
    dyntype_release(ctx, ret); // release duplicate map
    ret = dyntype_invoke(ctx, "set", obj, 2, argv + 2); // set str -> array
    dyntype_release(ctx, ret); // release duplicate map


    ret = dyntype_invoke(ctx, "has", obj, 1, argv);
    bool has;
    dyntype_to_bool(ctx, ret, &has);
    EXPECT_EQ(has, DYNTYPE_TRUE); // set success

    ret = dyntype_get_property(ctx, obj, "size");
    double sz;
    dyntype_to_number(ctx, ret, &sz);
    EXPECT_EQ(sz, 2.0); // size is 2

    ret = dyntype_invoke(ctx, "delete", obj, 1, argv); // delete num -> boolean
    dyntype_to_bool(ctx, ret, &has);
    EXPECT_EQ(has, DYNTYPE_TRUE); // delete success

    ret = dyntype_get_property(ctx, obj, "size");
    dyntype_to_number(ctx, ret, &sz);
    EXPECT_EQ(sz, 1.0); // size is 1

    ret = dyntype_invoke(ctx, "clear", obj, 0, argv); // clear
    ret = dyntype_get_property(ctx, obj, "size");
    dyntype_to_number(ctx, ret, &sz);
    EXPECT_EQ(sz, 0.0); // size is 0

    dyntype_release(ctx, num);
    dyntype_release(ctx, boolean);
    dyntype_release(ctx, str);
    dyntype_release(ctx, array);

    dyntype_release(ctx, obj);
}

static dyn_value_t
test_callback_dispatcher(void *exec_env_v, dyn_ctx_t ctx, void *vfunc, dyn_value_t this_obj,
                         int argc, dyn_value_t *args)
{
    return dyntype_new_boolean(dyntype_get_context(), true);
}

TEST_F(ObjectPropertyTest, map_callback_test)
{
    dyn_value_t obj = dyntype_new_object_with_class(ctx, "Map", 0, NULL);

    char str[] = { ' ', '\0' };
    dyn_value_t argv[10];
    dyn_value_t gkey;
    dyn_value_t ret;

    for (int i = 0, j = '0'; i < 10; i++, j++) {
        str[0] = j;
        dyn_value_t key = dyntype_new_string(ctx, str);
        dyn_value_t val = dyntype_new_number(ctx, i);
        argv[0] = key;
        argv[1] = val;
        ret = dyntype_invoke(ctx, "set", obj, 2, argv); // set  num -> boolean
        dyntype_release(ctx, ret);                      // release duplicate map
        dyntype_release(ctx, key);
        dyntype_release(ctx, val);
    }

    gkey = dyntype_new_string(ctx, str);
    argv[0] = gkey;
    ret = dyntype_invoke(ctx, "has", obj, 1, argv);
    bool has;
    dyntype_to_bool(ctx, ret, &has);
    EXPECT_EQ(has, DYNTYPE_TRUE); // set success

    ret = dyntype_get_property(ctx, obj, "size");
    double sz;
    dyntype_to_number(ctx, ret, &sz);
    EXPECT_EQ(sz, 10.0); // size is 10

    // compile error: undefine symbol dyntype_callback_for_js
    // temp fix: Replace with printing a mock statement at the calling location.
    dyn_value_t func = dyntype_new_extref(ctx, NULL, ExtFunc, NULL);
    EXPECT_EQ(dyntype_is_function(ctx, func), DYNTYPE_TRUE);

    argv[0] = func;
    ret = dyntype_invoke(ctx, "forEach", obj, 1, argv);

    /* We didn't register a callback dispatcher, so the return value should be
     * exception */
    EXPECT_TRUE(dyntype_is_exception(ctx, ret));

    dyntype_set_callback_dispatcher(ctx, test_callback_dispatcher);
    ret = dyntype_invoke(ctx, "forEach", obj, 1, argv);
    /* The forEach method should return undefined no matter what is returned by
     * the callback */
    EXPECT_TRUE(dyntype_is_undefined(ctx, ret));

    dyntype_release(ctx, ret);
    dyntype_release(ctx, func);
    dyntype_release(ctx, obj);
}
