#import <UIKit/UIKit.h>
#include <cstdlib>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

#ifdef PLATFORM_TEST_TAKES_ARGS
int PlatformTestEntry(int argc, char** argv);
#else
int PlatformTestEntry();
#endif

// 每次启动仅访问本次 UUID 对应的应用沙箱目录。CTest 读取同一 UUID 的原子结果。
static int RunPlatformTest(NSURL* scratch, NSArray<NSString*>* arguments, bool& created)
{
    NSFileManager* files = [NSFileManager defaultManager];
    if (![files createDirectoryAtURL:scratch withIntermediateDirectories:NO
        attributes:@{NSFilePosixPermissions:@0700} error:nullptr])
    {
        return 1;
    }
    created = true;
    const char* path = scratch.fileSystemRepresentation;
    if (chdir(path) != 0 || setenv("TMPDIR", path, 1) != 0 ||
        setenv("CSYSTEM_TEST_DIR", path, 1) != 0 || setenv("SHARED_MEMORY_TEST_DIR", path, 1) != 0)
    {
        return 1;
    }
#ifdef PLATFORM_TEST_TAKES_ARGS
    std::vector<std::string> values;
    for (NSString* arg in arguments)
    {
        values.emplace_back(arg.UTF8String);
        values.back().push_back('\0');
    }
    std::vector<char*> argv;
    for (std::string& value : values)
    {
        argv.push_back(&value[0]);
    }
    argv.push_back(nullptr);
    return PlatformTestEntry(static_cast<int>(values.size()), argv.data());
#else
    (void)arguments;
    return PlatformTestEntry();
#endif
}

@interface PlatformTestApp : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow* window;
@end

@implementation PlatformTestApp
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options
{
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    UIViewController* controller = [[UIViewController alloc] init];
    UILabel* label = [[UILabel alloc] initWithFrame:self.window.bounds];
    label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    label.textAlignment = NSTextAlignmentCenter;
    label.text = @"Running test";
    controller.view = label;
    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];

    NSArray<NSString*>* input = [NSProcessInfo processInfo].arguments;
    NSUInteger token = [input indexOfObject:@"--platform-test-run"];
    NSUUID* run = nil;
    if (token != NSNotFound && token + 1 < input.count)
    {
        run = [[NSUUID alloc] initWithUUIDString:input[token + 1]];
    }
    if (run == nil)
    {
        run = [NSUUID UUID];
    }
    NSMutableArray* arguments = [NSMutableArray arrayWithObject:input.firstObject ?: @"test"];
    NSUInteger start = [input indexOfObject:@"--platform-test-arguments"];
    if (start != NSNotFound)
    {
        [arguments addObjectsFromArray:[input subarrayWithRange:NSMakeRange(start + 1, input.count - start - 1)]];
    }
    NSURL* tmp = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSString* name = [@"platform-test-" stringByAppendingString:run.UUIDString.lowercaseString];
    NSURL* scratch = [tmp URLByAppendingPathComponent:name isDirectory:YES];
    NSURL* result = [tmp URLByAppendingPathComponent:[name stringByAppendingString:@".txt"]];
    NSURL* transcript = [tmp URLByAppendingPathComponent:[name stringByAppendingString:@".log"]];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        int status = 1;
        bool created = false;
        int file = open(transcript.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
        int savedOut = dup(STDOUT_FILENO);
        int savedErr = dup(STDERR_FILENO);
        fflush(nullptr);
        bool capture = file >= 0 && savedOut >= 0 && savedErr >= 0 && dup2(file, STDOUT_FILENO) >= 0 && dup2(file, STDERR_FILENO) >= 0;
        try
        {
            if (capture)
            {
                status = RunPlatformTest(scratch, arguments, created);
            }
        }
        catch (const std::exception& error)
        {
            std::fprintf(stderr, "C++ test exception: %s\n", error.what());
        }
        catch (...)
        {
            std::fputs("C++ test exception\n", stderr);
        }
        fflush(nullptr);
        if (savedOut >= 0)
        {
            dup2(savedOut, STDOUT_FILENO); close(savedOut);
        }
        if (savedErr >= 0)
        {
            dup2(savedErr, STDERR_FILENO); close(savedErr);
        }
        if (file >= 0)
        {
            close(file);
        }
        if (created)
        {
            chdir(tmp.fileSystemRepresentation);
            [[NSFileManager defaultManager] removeItemAtURL:scratch error:nullptr];
        }
        NSString* text = [NSString stringWithFormat:@"%d", status];
        NSMutableData* record = [[[text stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding] mutableCopy];
        if (file >= 0)
        {
            NSData* log = [NSData dataWithContentsOfURL:transcript];
            if (log)
            {
                [record appendData:log];
            }
            else
            {
                record = [NSMutableData dataWithData:[@"1\nMissing test transcript\n" dataUsingEncoding:NSUTF8StringEncoding]]; status = 1;
            }
            [[NSFileManager defaultManager] removeItemAtURL:transcript error:nullptr];
        }
        if (![record writeToURL:result atomically:YES])
        {
            NSLog(@"Unable to publish test result");
            status = 1;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            label.text = [NSString stringWithFormat:@"%@ (exit %d)", status == 0 ? @"PASS" : @"FAIL", status];
            NSLog(@"%@", label.text);
        });
    });
    return YES;
}
@end

int main(int argc, char** argv)
{
    @autoreleasepool
    {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([PlatformTestApp class]));
    }
}