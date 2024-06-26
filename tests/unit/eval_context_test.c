#include <test.h>

#include <eval_context.h>
#include <misc_lib.h>                                          /* xsnprintf */
#include <known_dirs.h>
#include <time_classes.h>

char CFWORKDIR[CF_BUFSIZE];

void tests_setup(void)
{
    static char env[] = /* Needs to be static for putenv() */
        "CFENGINE_TEST_OVERRIDE_WORKDIR=/tmp/CFENGINE_eval_context_test.XXXXXX";
    char *workdir = strchr(env, '=');
    assert(workdir && workdir[1] == '/');
    workdir++;

    mkdtemp(workdir);
    strlcpy(CFWORKDIR, workdir, CF_BUFSIZE);
    putenv(env);

    mkdir(GetStateDir(), 0766);
}

void tests_teardown(void)
{
    char cmd[CF_BUFSIZE];
    xsnprintf(cmd, CF_BUFSIZE, "rm -rf '%s'", CFWORKDIR);
    system(cmd);
}

static void test_class_persistence(void)
{
    EvalContext *ctx = EvalContextNew();

    // simulate old version
    {
        CF_DB *dbp;
        PersistentClassInfo i;
        assert_true(OpenDB(&dbp, dbid_state));

        i.expires = UINT_MAX;
        i.policy = CONTEXT_STATE_POLICY_RESET;

        WriteDB(dbp, "old", &i, sizeof(PersistentClassInfo));

        CloseDB(dbp);
    }

    // e.g. by monitoring
    EvalContextHeapPersistentSave(ctx, "class1", 3, CONTEXT_STATE_POLICY_PRESERVE, "a,b");

    // e.g. by a class promise in a bundle with a namespace
    {
        Policy *p = PolicyNew();
        Bundle *bp = PolicyAppendBundle(p, "ns1", "bundle1", "agent", NULL, NULL);

        EvalContextStackPushBundleFrame(ctx, bp, NULL, false);
        EvalContextHeapPersistentSave(ctx, "class2", 5, CONTEXT_STATE_POLICY_PRESERVE, "x");
        EvalContextStackPopFrame(ctx);

        PolicyDestroy(p);
    }

    EvalContextHeapPersistentLoadAll(ctx);

    {
        const Class *cls = EvalContextClassGet(ctx, "default", "old");
        assert_true(cls != NULL);

        assert_string_equal("old", cls->name);
        assert_true(cls->tags != NULL);
        assert_int_equal(1, StringSetSize(cls->tags));
        assert_true(StringSetContains(cls->tags, "source=persistent"));
    }

    {
        const Class *cls = EvalContextClassGet(ctx, "default", "class1");
        assert_true(cls != NULL);

        assert_string_equal("class1", cls->name);
        assert_true(cls->tags != NULL);
        assert_int_equal(3, StringSetSize(cls->tags));
        assert_true(StringSetContains(cls->tags, "source=persistent"));
        assert_true(StringSetContains(cls->tags, "a"));
        assert_true(StringSetContains(cls->tags, "b"));
    }

    {
        const Class *cls = EvalContextClassGet(ctx, "ns1", "class2");
        assert_true(cls != NULL);

        assert_string_equal("ns1", cls->ns);
        assert_string_equal("class2", cls->name);
        assert_true(cls->tags != NULL);
        assert_int_equal(2, StringSetSize(cls->tags));
        assert_true(StringSetContains(cls->tags, "source=persistent"));
        assert_true(StringSetContains(cls->tags, "x"));
    }

    EvalContextDestroy(ctx);
}

void test_changes_chroot(void)
{
    /* Should add '/' to the end implicitly. */
    SetChangesChroot("/changes/go/here");

    /* The most trivial case. */
    const char *chrooted = ToChangesChroot("/etc/issue");
    assert_string_equal(chrooted, "/changes/go/here/etc/issue");

    /* A shorter path to test that NUL-byte is added/copied properly. */
    chrooted = ToChangesChroot("/etc/ab");
    assert_string_equal(chrooted, "/changes/go/here/etc/ab");

    /* And a longer path again. */
    chrooted = ToChangesChroot("/etc/sysctl.d/00-default.conf");
    assert_string_equal(chrooted, "/changes/go/here/etc/sysctl.d/00-default.conf");

#ifndef __MINGW32__
    /* Inverse should work as expected */
    const char *normal = ToNormalRoot(chrooted);
    assert_string_equal(normal, "/etc/sysctl.d/00-default.conf");
#endif
}

void test_eval_with_token_from_list(void)
{
    /* The timestamp should generate these classes (among others):
     *  'GMT_Afternoon', 'GMT_Day29', 'GMT_Hr13', 'GMT_Hr13_Q3', 'GMT_Lcycle_2',
     *  'GMT_May', 'GMT_Min30_35', 'GMT_Min33', 'GMT_Q3', 'GMT_Wednesday',
     *  'GMT_Yr2024'
     */
    const time_t timestamp = 1716989621;
    StringSet *time_classes = GetTimeClasses(timestamp);
    StringSetIterator iter = StringSetIteratorInit(time_classes);
    const char *time_class = NULL;
    while((time_class = StringSetIteratorNext(&iter)) != NULL) {
        printf("Time class: %s\n", time_class);
    }

    assert_true(EvalWithTokenFromList("GMT_Wednesday", time_classes));
    assert_false(EvalWithTokenFromList("GMT_Monday", time_classes));
    assert_false(EvalWithTokenFromList("GMT_Wednesday.GMT_Monday", time_classes));
    assert_true(EvalWithTokenFromList("GMT_Monday|GMT_Wednesday", time_classes));
    assert_true(EvalWithTokenFromList("!GMT_Monday", time_classes));

    StringSetDestroy(time_classes);
}

int main()
{
    PRINT_TEST_BANNER();
    tests_setup();

    const UnitTest tests[] =
    {
        unit_test(test_class_persistence),
        unit_test(test_changes_chroot),
        unit_test(test_eval_with_token_from_list),
    };

    int ret = run_tests(tests);

    tests_teardown();

    return ret;
}

