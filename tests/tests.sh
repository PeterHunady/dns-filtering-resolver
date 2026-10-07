# file: tests/tests.sh

# Author: Peter Huňady
# Login: xhunadp00

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0

TEST_DIR="tests"
mkdir -p "$TEST_DIR"


FILTER_FILE="$TEST_DIR/test.txt"
cat > "$FILTER_FILE" << 'EOF'
www.google.com
facebook.com
org.wikipedia.en
twitter.com
www.example.github.com
subdomain.youtube.com
example.com
fit.vut.cz
www.instagram.com
EOF


EMPTY_FILTER_FILE="$TEST_DIR/test2.txt"
cat > "$EMPTY_FILTER_FILE" << 'EOF'
EOF


FILTER_WITH_COMMENTS="$TEST_DIR/test3.txt"
cat > "$FILTER_WITH_COMMENTS" << 'EOF'
example.com

  www.example.github.com
www.google.com  
# twitter.com
#facebook.com
org.wikipedia.en  

     subdomain.youtube.com     

EOF


FILTER_TEST_FILE="$TEST_DIR/test4.txt"
cat > "$FILTER_TEST_FILE" << 'EOF'
www.google.com
facebook.com
org.wikipedia.en
twitter.com
#random.domain.com
www.admin.github.com
#  example.com


#fit.vut.cz
www.instagram.com

     subdomain.youtube.com     

EOF



run_valid_test(){
    local test_name="$1"
    local expected_exit_code="$2"
    shift 2
    local cmd="$@"

    printf "$test_name: "

    $cmd > /dev/null 2>&1 &
    actual_exit_code=$?
    local pid=$!

    sleep 1

    if kill -0 "$pid" 2>/dev/null; then
        kill "$pid" >/dev/null 2>&1
        wait "$pid" 2>/dev/null
        if [ $expected_exit_code -eq 0 ]; then
            printf "${GREEN}PASS${NC}\n"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            printf "${RED}FAIL${NC} (process running, expected exit code $expected_exit_code)\n"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    else
        wait "$pid" 2>/dev/null
        actual_exit_code=$?
        if [ $actual_exit_code -eq $expected_exit_code ]; then
            printf "${GREEN}PASS${NC}\n"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            printf "${RED}FAIL${NC} (expected $expected_exit_code, got $actual_exit_code)\n"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    fi
}

run_parameters_test() {
    local test_name="$1"
    local expected_exit_code="$2"
    shift 2
    local cmd="$@"

    printf "$test_name: "

    $cmd > /dev/null 2>&1
    actual_exit_code=$?
    
    if [ $actual_exit_code -eq $expected_exit_code ]; then
        printf "${GREEN}PASS${NC}\n"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "${RED}FAIL${NC} (expected $expected_exit_code, got $actual_exit_code)\n"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

run_filter_file_test() {
    local test_name="$1"
    local expected_exit_code="$2"
    shift
    local cmd="$@"

    printf "$test_name: "

    $cmd > /dev/null 2>&1 &
    local pid=$!

    sleep 1

    if kill -0 "$pid" 2>/dev/null; then
        kill "$pid" >/dev/null 2>&1
        wait "$pid" 2>/dev/null
        printf "${GREEN}PASS${NC}\n"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        wait "$pid" 2>/dev/null
        local code=$?
        printf "${RED}FAIL${NC} (process exited early, code $code)\n"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

run_dns_filtering_test() {
    local test_name="$1"
    local domain="$2"
    local qtype="$3"
    local expected_status="$4"
    local port="$5"

    printf "$test_name: "

    actual_status=$(dig @127.0.0.1 -p "$port" "$domain" "$qtype" 2>&1 | grep "status:" | awk '{print $6}' | tr -d ',')

    if [ "$actual_status" = "$expected_status" ]; then
        printf "${GREEN}PASS${NC}\n"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "${RED}FAIL${NC} (expected $expected_status, got $actual_status)\n"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}



echo ""
echo "--------------------------------"
echo "  DNS Resolver - Valid Tests "
echo "--------------------------------"
echo ""

run_valid_test "No arguments" 0 ./dns
run_valid_test "Help parameter (-h)" 0 ./dns -h
run_valid_test "Help parameter (--help)" 0 ./dns --help
run_valid_test "All parameters" 0 ./dns -s 8.8.8.8 -f "$FILTER_FILE" -p 10543
run_valid_test "All parameters with a different order" 0 ./dns -p 10543 -f "$FILTER_FILE" -s 8.8.8.8
run_valid_test "IPv6 DNS server" 0 ./dns -s 2001:4860:4860::8888 -p 10543 -f "$FILTER_FILE"



echo ""
echo "--------------------------------"
echo "  DNS Resolver - Argument Tests "
echo "--------------------------------"
echo ""

run_parameters_test "Invalid parameter" 1 ./dns -s 8.8.8.8 -f "$FILTER_FILE" -x
run_parameters_test "Missing server value" 2 ./dns -f "$FILTER_FILE" -s
run_parameters_test "Port out of range (negative)" 3 ./dns -s 8.8.8.8 -p -1 -f "$FILTER_FILE"
run_parameters_test "Port out of range (> 65535)" 3 ./dns -s 8.8.8.8 -p 65536 -f "$FILTER_FILE"
run_parameters_test "Missing filter file value" 4 ./dns -s 8.8.8.8 -f
run_parameters_test "Missing server (-s)" 5 ./dns -f "$FILTER_FILE"
run_parameters_test "Missing filter file (-f)" 6 ./dns -s 8.8.8.8
run_parameters_test "Duplicate server parameter" 7 ./dns -s 1.1.1.1 -s 8.8.8.8 -f "$FILTER_FILE"
run_parameters_test "Duplicate port parameter" 7 ./dns -s 8.8.8.8 -p 10543 -p 8080 -f "$FILTER_FILE"
run_parameters_test "Duplicate filter file parameter" 7 ./dns -s 8.8.8.8 -f "$FILTER_FILE" -f "$EMPTY_FILTER_FILE"
run_parameters_test "Non-existent filter file" 8 ./dns -s 8.8.8.8 -f some_file.txt -p 10543
run_parameters_test "Invalid IPv4 of DNS server" 13 ./dns -s 8.8.832.8 -f "$FILTER_FILE" -p 10543
run_parameters_test "Invalid IPv4 of DNS server" 13 ./dns -s 8.8.c.8 -f "$FILTER_FILE" -p 10543
run_parameters_test "Invalid IPv6 of DNS server" 13 ./dns -s 2001:0db8:130f:0000:0000:09c0:876a:130u -f "$FILTER_FILE" -p 10543
run_parameters_test "Invalid domain name of DNS server" 13 ./dns -s g00gle.com -f "$FILTER_FILE" -p 10543



echo ""
echo "--------------------------------"
echo "DNS Resolver - Filter File Tests"
echo "--------------------------------"
echo ""

run_parameters_test "Empty filter file" 9 ./dns -s 8.8.8.8 -f "$EMPTY_FILTER_FILE" -p 10543
run_filter_file_test "Valid filter file" ./dns -s 8.8.8.8 -f "$FILTER_FILE" -p 10543
run_filter_file_test "Filter file with comments and whitespaces" ./dns -s 8.8.8.8 -f "$FILTER_WITH_COMMENTS" -p 10543



echo ""
echo "--------------------------------"
echo "       DNS Filtering Test       "
echo "--------------------------------"
echo ""

DNS_PORT=10543
./dns -s 8.8.8.8 -f "$FILTER_TEST_FILE" -p $DNS_PORT > /dev/null 2>&1 &
DNS_PID=$!
sleep 2

if ! kill -0 "$DNS_PID" 2>/dev/null; then
    printf "${RED}FAIL: DNS resolver failed to start${NC}\n"
    FAIL_COUNT=$((FAIL_COUNT + 25))
else
    run_dns_filtering_test "Filtered domain (twitter.com)" "twitter.com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Subdomain of filtered domain (www.twitter.com)" "www.twitter.com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Unfiltered unrelated domain (www.org.google.com)" "www.org.google.com" "A" "NXDOMAIN" $DNS_PORT
    run_dns_filtering_test "Changed top level domain (twitter.cz)" "twitter.cz" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Exact match of filtered subdomain (subdomain.youtube.com)" "subdomain.youtube.com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Parent domain of filtered subdomain (youtube.com)" "youtube.com" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Exact match of filtered deep subdomain (www.admin.github.com)" "www.admin.github.com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Different subdomain path (www.github.com)" "www.github.com" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Non-filtered domain (vut.cz)" "vut.cz" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Commented line (example.com)" "example.com" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Commented line (fit.vut.cz)" "fit.vut.cz" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Multiple subdomains (example.admin.org.facebook.com)" "example.admin.org.facebook.com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Subdomain of filtered subdomain (admin.subdomain.youtube.com)" "admin.subdomain.youtube.com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Similar domain (www.ggoogle.com)" "www.ggoogle.com" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Similar domain (ttwitter.com)" "ttwitter.com" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Case-insensitive (TwiTTer.Com)" "TwiTTer.Com" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Case-insensitive (wWW.gOoGlE.cOm)" "wWW.gOoGlE.cOm" "A" "REFUSED" $DNS_PORT
    run_dns_filtering_test "Case-insensitive (vUT.cZ)" "vUT.cz" "A" "NOERROR" $DNS_PORT
    run_dns_filtering_test "Not implemented AAAA type of querry filtered domain (twitter.com)" "twitter.com" "AAAA" "NOTIMP" $DNS_PORT
    run_dns_filtering_test "Not implemented MX type of querry (vut.cz)" "vut.cz" "MX" "NOTIMP" $DNS_PORT
    run_dns_filtering_test "Not implemented CNAME type of querry (github.com)" "github.com" "CNAME" "NOTIMP" $DNS_PORT
    run_dns_filtering_test "Not implemented TXT type of querry (google.com)" "google.com" "TXT" "NOTIMP" $DNS_PORT
    run_dns_filtering_test "Not implemented NS type of querry (example.com)" "example.com" "NS" "NOTIMP" $DNS_PORT
    run_dns_filtering_test "Not implemented SOA type of querry (vut.cz)" "vut.cz" "SOA" "NOTIMP" $DNS_PORT
    run_dns_filtering_test "Non-existent domain (whuqdhuewfbjhsabuefugbwzu231231.com)" "whuqdhuewfbjhsabuefugbwzu231231.com" "A" "NXDOMAIN" $DNS_PORT

    kill "$DNS_PID" 2>/dev/null
    wait "$DNS_PID" 2>/dev/null
fi



echo ""
echo "--------------------------------"
echo "     Multiple Queries Test      "
echo "--------------------------------"
echo ""

./dns -s 8.8.8.8 -f "$FILTER_TEST_FILE" -p $DNS_PORT > /dev/null 2>&1 &
DNS_PID=$!
sleep 2

if ! kill -0 "$DNS_PID" 2>/dev/null; then
    printf "${RED}FAIL: DNS resolver failed to start${NC}\n"
    FAIL_COUNT=$((FAIL_COUNT + 1))
else
    printf "Multiple queries: "

    (
        dig @127.0.0.1 -p "$DNS_PORT" google.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" facebook.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" vut.cz A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" github.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" stackoverflow.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" reddit.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" twitter.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" youtube.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" amazon.com A > /dev/null 2>&1 &
        dig @127.0.0.1 -p "$DNS_PORT" netflix.com A > /dev/null 2>&1 &
        wait
    ) &
    C_PID=$!

    COUNTER=0
    while kill -0 "$C_PID" 2>/dev/null && [ $COUNTER -lt 10 ]; do
        sleep 1
        COUNTER=$((COUNTER + 1))
    done

    if kill -0 "$C_PID" 2>/dev/null; then
        kill "$C_PID" 2>/dev/null
        wait "$C_PID" 2>/dev/null
    fi

    if kill -0 "$DNS_PID" 2>/dev/null; then
        printf "${GREEN}PASS${NC}\n"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "${RED}FAIL${NC} (resolver crashed during concurrent queries)\n"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    kill "$DNS_PID" 2>/dev/null
    wait "$DNS_PID" 2>/dev/null
fi



echo ""
echo "--------------------------------"
echo "          SERVFAIL Test         "
echo "--------------------------------"
echo ""

printf "SERVFAIL with non-responsive DNS server (10.0.11.111): "
./dns -s 10.0.11.111 -f "$FILTER_TEST_FILE" -p "$DNS_PORT" > /dev/null 2>&1 &
DNS_PID=$!
sleep 2

if ! kill -0 "$DNS_PID" 2>/dev/null; then
    printf "${RED}FAIL${NC} (resolver failed to start)\n"
    FAIL_COUNT=$((FAIL_COUNT + 1))
else
    STATUS=$(dig @127.0.0.1 -p "$DNS_PORT" vut.cz A +time=10 +tries=1 2>&1 | grep "status:" | awk '{print $6}' | tr -d ',')

    if [ "$STATUS" = "SERVFAIL" ]; then
        printf "${GREEN}PASS${NC}\n"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        printf "${RED}FAIL${NC} (expected SERVFAIL, got: $STATUS)\n"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi

    kill "$DNS_PID" 2>/dev/null
    wait "$DNS_PID" 2>/dev/null
fi



rm -f "$FILTER_FILE" "$EMPTY_FILTER_FILE" "$FILTER_WITH_COMMENTS" "$FILTER_TEST_FILE"


echo ""
echo "================================"
echo "            Summary             "
echo "================================"
printf "${GREEN}PASSED: $PASS_COUNT${NC}\n"
printf "${RED}FAILED: $FAIL_COUNT${NC}\n"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi
