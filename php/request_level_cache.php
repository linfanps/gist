<?php
/** 
  * example:
  * get_cache('key1');
  * get_cache(['key1', 'key2']);
  */
function get($keys)
{
    //使用静态变量，使其作用于整个Request期间
    static $local_cache = []; //本地cache
    static $memcached; //memcached句柄

    $keys = (array)$keys;

    $result = $missing = [];
    foreach ($keys as $key) {
        if (isset($local_cache[$key])) {
            $result[$key] = $cache[$key];
        //本地cache中没有缓存的key
        } else {
            $missing[] = $key;
        }
    }

    if (!$missing) {
        return $result;
    }

    if (!$memcached) {
        $memcached = new Memcached;
    }
    //本地cache中没有命中的key，从memcached中一次性取出所有键值返回并存到本地cache
    $mresult = $memcached->getMulti($missing);
    if ($mresult) {
        foreach ($mresult as $key => $value) {
            $result[$key] = $value;
            $local_cache[$key] = $value;
        }
    }

    return $result;
}
?>



