<?php
include 'simple_html_dom.php'; //解析HTML文档库 http://www.phpddt.com/manual/simplehtmldom_1_5/manual.htm

class URLFetchTool {

    public $total_http_request_num;
    public $total_http_download_size;
    
    private $current_fetch_url;
    private $current_fetch_url_compents;
    private $wait_to_fetch_urls;
    private $debug;

    public function __construct($url) 
    {
        $this->total_http_request_num = 0; //总HTTP请求数
        $this->total_http_download_size = 0; //总下载字节数
        $this->debug = false; //默认不处于debug模式，不输出FETCH过程
        $this->wait_to_fetch_urls = array($url); //等待进行HTTP请求的URL数组
        $this->current_fetch_url = '';//当前正在进行HTTP请求的URL
        $this->current_fetch_url_compents = array(); //当前进行HTTP请求URL组件,parse_url结果
    }

    public function set_debug($is_debug) 
    {
        $this->debug = $is_debug;
    }

    public function fetch()
    {
        $visited_urls = array(); 
        while (!empty($this->wait_to_fetch_urls)) {
            //每次循环从队头取出URL获取其内容，若页面内容中有需要再次进行HTTP请求的URL放入队尾
            $this->current_fetch_url = array_shift($this->wait_to_fetch_urls);
            //检查URL格式是否正确
            if (!$this->check_url_validation()) {
                echo "URL [$this->current_fetch_url] malformed!\n";
                continue;
            }
            //为避免形成访问环，进行过HTTP请求的URL不再请求
            if (in_array($this->current_fetch_url, $visited_urls)) {
                if ($this->debug)
                    echo "Fetch URL [$this->current_fetch_url] completed, URL has already been fetched\n";
                continue;
            }

            $res = $this->get_url_content();
            array_push($visited_urls, $this->current_fetch_url);
            if ($res !== false) {
                if ($this->debug) {
                    echo "Fetch URL [$this->current_fetch_url] completed, fetch_size:".$res['response']['size_download'].
                        ", response_code:".$res['response']['http_code']."\n";
                }
                $this->total_http_request_num += 1;
                $this->total_http_download_size += $res['response']['size_download'];
                //访问URL为HTML页面,解析页面，继续请求页面中css/javascript/image/iframe等
                if (intval($res['response']['http_code']) == 200 
                    && strpos($res['response']['content_type'], 'text/html') !== FALSE) {
                    $this->strip_links($res['content']);
                }
            }
        }

    }
    //使用CURL获取URL内容
    private function get_url_content() 
    {
        $https_protocol = $this->current_fetch_url_compents['scheme'] == 'https';
        $user_agent ='Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.71 Safari/537.36';
        $timeout = 5;
        $ch = curl_init();

        curl_setopt($ch, CURLOPT_URL, $this->current_fetch_url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION,1);
        curl_setopt($ch, CURLOPT_TIMEOUT, $timeout);
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, $https_protocol);
        curl_setopt($ch, CURLOPT_USERAGENT, $user_agent);

        $content = curl_exec($ch);

        if (curl_errno($ch)) {
            echo "Curl $url error:".curl_error($ch)."\n";
            return false;
        }
        $response = curl_getinfo($ch);
        curl_close ($ch);

        return array('content' => $content, 'response' => $response);
    }
    //解析HTML内容，获取需要再次进行HTTP请求的资源链接
    //不处理js文件中引入的js，css中引入的css文件
    private function strip_links($content)
    {
        $html = str_get_html($content);
        //图片资源 <img src="...." /> 
        foreach ($html->find('img') as $ele) {
            if (!empty($ele->src)) {
                array_push($this->wait_to_fetch_urls, $this->expand_links($ele->src));
            }
        }
        //脚本资源,一般是引入外部javascript文件 <script type="text/javascript" src=""></script> 
        foreach ($html->find('script') as $ele) {
            if (!empty($ele->src)) {
                array_push($this->wait_to_fetch_urls, $this->expand_links($ele->src));
            }
        }
        //css资源 <link rel="stylesheet" href = ""> 
        foreach ($html->find('link') as $ele) {
            if ($ele->rel == "stylesheet" && !empty($ele->href)) {
                array_push($this->wait_to_fetch_urls, $this->expand_links($ele->href));
            }
        }

        //frame/iframe资源 <[frame|iframe] src="">
        //音视频资源 <[audio|video|source] src="">
        //嵌入外部资源 <embed src="">
        foreach ($html->find('frame, iframe, audio, video, source, embed') as $ele) {
            if (!empty($ele->src)) {
                array_push($this->wait_to_fetch_urls, $this->expand_links($ele->src));
            }
        }

        //清理，释放内存
        $html->clear();

    }
    //根据访问的URL补全其内容的中的链接地址
    //URL: <scheme>://<user>:<pass>@<host>:<port>/<path>;<params>?<query>#<frag>
    private function expand_links($link)
    {
        $link = strtolower($link);
        //完整链接
        if (substr($link, 0, 7) == "http://" || substr($link, 0, 8) == "https://") {
            return $link;
        }
        //缺少<scheme>的链接，//www.xxx.com/xxx/,默认scheme为http
        if (substr($link, 0, 2) == "//") {
            return "http:$link";
        }
        //相对于this->current_fetch_url的绝对路径链接, /<path>;<params>?<query>#<frag>, 取this->current_fetch_url的/<path>前段补全
        if ($link[0] == '/') {
            $pieces = explode("/", $this->current_fetch_url);
            $fill_url_info = implode("/", array_slice($pieces, 0, 3));
            return "$fill_url_info$link";
        }

        //相对于this->current_fetch_url的相对路径,取this->current_fetch_url最后/之前信息补全
        $pos = strrpos($this->current_fetch_url, "/");
        $fill_url_info = substr($this->current_fetch_url, 0, $pos);
        return "$fill_url_info/$link";

    }
    //检查url的合法性
    private function check_url_validation()
    {
        $this->current_fetch_url_compents = parse_url($this->current_fetch_url);
        if ($this->current_fetch_url_compents === FALSE) {
            return false;
        }
        //procotol 默认为http,且只支持http和https
        if (empty($this->current_fetch_url_compents['scheme'])) {
            $this->current_fetch_url_compents['scheme'] = 'http';
            $this->current_fetch_url = "http://$this->current_fetch_url"; //为下面调用filter_var服务,filter_var对无scheme格式的URL判断为无效URL
        }
        if ($this->current_fetch_url_compents['scheme'] != 'http' && $this->current_fetch_url_compents['scheme'] != 'https') {
            return false;
        }
        if (filter_var($this->current_fetch_url, FILTER_VALIDATE_URL) === FALSE) {
            return false;
        }

        return true;
    }
}

if ($argc == 1) {
    echo "No URL specified!\nUsage: php url_analysis.php <url> [debug_info]";
    exit;
}

$urltool = new URLFetchTool(trim($argv[1]));
if (isset($argv[2])) {
    $urltool->set_debug(true);
}
$urltool->fetch();
echo "total_http_request_num = $urltool->total_http_request_num times, total_http_download_size = $urltool->total_http_download_size bytes\n";

?>
