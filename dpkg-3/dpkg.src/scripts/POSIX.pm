sub ENOENT {
    return 2;
}

sub SIGPIPE {
    return 13;
}

sub WIFSTOPPED {
    my ($status) = @_;
    my ($_status) = $status & 0177;
    my ($ret) = ($_status == 0177);
    #print "WIFSTOPPED ($status) -> $ret\n";
    return $ret;
}    

sub WIFSIGNALED {
    my ($status) = @_;
    my ($_status) = $status & 0177;
    my ($ret) = (($_status != 0177) && ($_status != 0));
    #print "WIFSIGNALED ($status) -> $ret\n";
    return $ret;
}

sub WTERMSIG {
    my ($status) = @_;
    my ($ret) = $status & 0177;
    #print "WTERMSIG ($status) -> $ret\n";
    return $ret;
}

sub WIFEXITED {
    my ($status) = @_;
    my ($_status) = $status & 0177;
    my ($ret) = ($_status == 0);
    #print "WIFEXITED ($status) -> $ret\n";
    return $ret;
}

sub WEXITSTATUS {
    my ($status) = @_;
    my ($ret) = $status >> 8;
    #print "WEXITSTATUS ($status) -> $ret\n";
    return $ret;
}

1;
