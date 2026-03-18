
#include <shim/cli.h>

#include <iostream>


int main(int argc, char* argv[]) {

    shm::cli cli("vcs", "A modern version control system");
    cli.version("1.0.0");

    cli.flag('v', "verbose").help("Enable verbose output").repeatable();
    cli.flag("no-color").help("Disable colored output");
    cli.option('C', "directory").help("Run as if started in this directory").defaults(".");

    auto& init = cli.subcommand("init", "Initialise a new repository");
    init.flag('q', "quiet").help("Only print error messages");
    init.option('b', "branch").help("Initial branch name").defaults("main");

    auto& add = cli.subcommand("add", "Add file contents to the index");
    add.flag('f', "force").help("Allow adding otherwise ignored files");
    add.flag('n', "dry-run").help("Do not actually add files, just show what would happen");
    add.positional("path").help("Path to file or directory to add");

    auto& commit = cli.subcommand("commit", "Record changes to the repository");
    commit.flag('a', "all").help("Automatically stage modified and deleted files");
    commit.flag('v', "verbose").help("Show unified diff of the commit").repeatable();
    commit.option('m', "message").help("Commit message").required();
    commit.option('A', "author").help("Override the commit author");

    auto& remote = cli.subcommand("remote", "Manage set of tracked repositories");
    remote.flag('v', "verbose").help("Show remote url after name");

    auto& remote_add = remote.subcommand("add", "Add a new remote");
    remote_add.flag('f', "fetch").help("Fetch the remote after adding");
    remote_add.option('t', "track").help("Track only a specific branch").repeatable();
    remote_add.positional("name").help("Name for the new remote");
    remote_add.positional("url").help("URL for the new remote");





    // Test
    char* args[] = {"program.exe", "--help"};
    auto testres = cli.parse(_countof(args), args);

    //auto result = cli.parse(argc, argv);

    // Debug, print out all of result
    //result.dump();

    return 0;
}
