var cmd_history_arr=[];
var cmd_title="console";
var cmd_path="/";
var cmd_history_addr=0;
var cmds=0;

function console_parse_cmd()
{
    cmds=JSON.parse(this.responseText);
}

function link_cmds_lib()
{
    var cmd_json_xhr=new XMLHttpRequest();
    cmd_json_xhr.onload=console_parse_cmd;
    cmd_json_xhr.onerror=function()
    {
        post_notification("cmds:命令库初始连接失败");
        append_src_req(link_cmds_lib);
    }
    cmd_json_xhr.open("GET", "cmdsss.json", true);
    cmd_json_xhr.send();
}

//运行命令
function run_cmd(event)
{
    console.log(event.keyCode)
    if(event.keyCode==13)
    {
        var cmd = document.getElementById("cmd_text").innerText;
        cmd=cmd.replaceAll("\n","");
        while(cmd[0]==0 || cmd[0]==" ")
        {
            cmd=cmd.substring(1);
        }
        cmd_history_arr.push(cmd);
        cmd_history_addr=0;
        document.getElementById("cmd_history").innerHTML+=cmd_path+">"+cmd+"<br />"+cmd_handle(cmd)+"<br />";
        
        setTimeout(() => {
            document.getElementById("cmd_text").innerText=" ";
        }, 10);
        return 0;
    }
    if(event.keyCode==38)
    {
        if(cmd_history_addr<cmd_history_arr.length-1)
        {
            document.getElementById("cmd_text").innerText=cmd_history_arr[cmd_history_addr++];
        }
        if(cmd_history_addr=cmd_history_arr.length-1)
        {
            document.getElementById("cmd_text").innerText=cmd_history_arr[cmd_history_addr];
        }
    }
    if(event.keyCode==40)
    {
        if(cmd_history_addr>0)
        {
            document.getElementById("cmd_text").innerText=cmd_history_arr[--cmd_history_addr];
        }
    }
    return event;
}

/**
* @function 识别命令，返回相关命令
* @param {string} cmd_first_word 识别词
* @param  {bool} full_match_only 是否全字识别（只识别唯一完全相同）
* @return {Object} 返回对象/数组，全字识别时返回对象，非时返回数组
*/
function recognize_cmd(cmd_first_word,full_match_only)
{
    if(full_match_only)
    {
        for(num=0;num<cmds.cmds.length;num++)
        {
            cmd_names=cmds.cmds[num].sub_names.split(" ");
            for(num2=0;num2<cmd_names.length;num2++)
            {
                if(cmd_first_word==cmd_names[num2])
                {
                    return cmds.cmds[num];
                }
            }
        }
        return false;
    }
    else
    {
        var results=[]
        for(num=0;num<cmds.cmds.length;num++)
        {
            cmd_names=cmds.cmds[num].sub_names.split(" ");
            for(num2=0;num2<cmd_names.length;num2++)
            {
                if(cmd_names[num2].indexOf(cmd_first_word)>-1)
                {
                    results.push(cmds.cmds[num])
                    break;
                }
            }
        }
        if(results.length==0)
        {
            return false;
        }
        return results;
    }
}

/**
 * 
 * @param {String} cmd 命令行字符串
 * @returns 返回结果字符串
 */
function cmd_handle(cmd)
{
    if(cmds==0||cmds==undefined)
    {
        return "命令库不存在,已取消本次命令,请检查与服务器连接";
    }
    var cmd_name=cmd.indexOf(" ")>0?cmd.substring(0,cmd.indexOf(" ")):cmd_name=cmd;
    var cmd_info=recognize_cmd(cmd_name,true);
    if(cmd_info)
    {
        switch(cmd_info.name)
        {
            case "目录":
                return cmd_menu();
                break;
            case "戒备":
                return cmd_set_state(cmd);
                break;
            case "路径":
                return cmd_set_path(cmd);
                break;
            case "echo":
                return cmd.substring(cmd.indexOf(" ")+1);
                break;
            case "标题":
                return cmd_set_title(cmd);
                break;
            case "清屏":
                return cmd_cls();
                break;
            case "颜色":
                return cmd_set_color(cmd);
                break;
            case "同步":
                return cmd_set_sync(cmd);
                break;
            case "重连":
                return cmd_set_retry_time(cmd);
                break;
            case "指令":
                return cmd_server_cmd(cmd);
                break;
            default:
                return "错误，请检指令处理函数代{left_bar.js}[cmd_handle]switch";
                break;
        }
    }
    else
    {
        cmd_info=recognize_cmd(cmd_name,false);
        if(cmd_info)
        {
            var names=""
            for(var num=0;num<cmd_info.length;num++)
            {
                names+=' "'+cmd_info[num].name+'" ';
            }
            return "无匹配指令，你要的指令可能是: "+names;
        }
        else
        {
            return '没有相关或相似指令，请输入"提示"获取指令列表';
        }
    }
}

function cmd_menu()
{
    var names="";
    for(num=0;num<cmds.cmds.length;num++)
    {
        names+=' "'+cmds.cmds[num].name+'" ';
    }
    return "支持的命令有:"+names;
}

function cmd_set_state(cmd)
{
    var params=cmd.split(" ")
    if(params[1].indexOf("零")>-1 || params[1].indexOf("0")>-1 || params[1].indexOf("常")>-1)
    {
        set_state_to_server(0);
        return "执行成功";
    }
    else if(params[1].indexOf("一")>-1 || params[1].indexOf("1")>-1)
    {
        set_state_to_server(1);
        return "执行成功";
    }
    else if(params[1].indexOf("二")>-1 || params[1].indexOf("2")>-1)
    {
        set_state_to_server(2);
        return "执行成功";
    }
    else if(params[1].indexOf("三")>-1 || params[1].indexOf("3")>-1)
    {
        set_state_to_server(3);
        return "执行成功";
    }
    else if(params[1].indexOf("四")>-1 || params[1].indexOf("4")>-1 || params[1].indexOf("休闲")>-1)
    {
        set_state_to_server(4);
        return "执行成功";
    }
    return "参数错误，示例: '戒备 一级戒备状态'";
}

function cmd_set_path(cmd)
{
    cmd_path=cmd.substring(cmd.indexOf(" ")+1);
    document.getElementById("cmd_text").style.setProperty("--path_str","'"+cmd_path+">'");
    return "路径设置成功";
}

function cmd_set_title(cmd)
{
    cmd_title=cmd.substring(cmd.indexOf(" ")+1);
    document.getElementById("console_title").innerText=cmd_title;
    return "标题设置成功";
}

function cmd_cls()
{
    setTimeout(() => {
        document.getElementById("cmd_history").innerHTML="SUPERNOVA CONSOLE [vision: 10.0.19044.1826]<br />SP-Ex.Co.Ltd 保留所有权利<br />";
    }, 20);
    return "";
}

function cmd_set_color(cmd)
{
    document.getElementById("command").style.setProperty("--text_color",cmd.substring(cmd.indexOf(" ")+1));
    return "颜色设置成功";
}

function cmd_set_sync(cmd)
{
    rgn_word=cmd.substring(cmd.indexOf(" ")+1);
    if(rgn_word=="同步"||rgn_word=="运行")
    {
        if(sync_handle==0)
        {
            sync_handle=setInterval(get_state_from_server,1000);
            return "成功";
        }
        else
        {
            return "同步定时函数句柄未清除";
        }
    }
    else if(rgn_word=="停止")
    {
        if(sync_handle)
        {
            clearInterval(sync_handle);
            sync_handle=0;
            return "停止成功";
        }
        else
        {
            return "无需重复清除";
        }
    }
    else
    {
        return "参数错误,参数应为“同步” “运行” “停止”";
    }
}

function cmd_set_retry_time(cmd)
{
    rgn_word=cmd.substring(cmd.indexOf(" ")+1);
    var rgn_time=Number(rgn_word);
    if(rgn_time!=NaN)
    {
        retry_time=rgn_time;
        return "成功";
    }
    return "语法错误,示例(10s):重连 10";
}

function cmd_reflesh_sucache(cmd)
{
    
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        // 输出接收到的文字数据
        post_notification("周数计算开始日期:"+xhr.responseText,"red");
        var beg_time=new Date(xhr.responseText.replaceAll("-","/"));
        diff_week = Math.floor((now_t.getTime()-beg_time.getTime())/(1000*3600*24*7))+1;

        var courses_list=document.getElementById("course_"+week_day_name_eng[week_day]+"day").getElementsByClassName("cource_grid");
        for(var num=0;num<courses_list.length;num++)
        {
            var fra=courses_list[num].getElementsByClassName("course_week_period");
            if(fra.length)
            {
                var src_text = fra[0].innerText;
                src_text.replace("周","");
                var parr = src_text.split("-");
                if(diff_week<Number(parr[0])||diff_week>Number(parr[1]))
                {
                    courses_list[num].getElementsByClassName("course_name")[0].style.color="darkgoldenrod";
                    courses_list[num].getElementsByClassName("course_site")[0].style.color="peru";
                }
                else
                {
                    courses_list[num].getElementsByClassName("course_name")[0].style.color="yellow";
                    courses_list[num].getElementsByClassName("course_site")[0].style.color="gold";
                }
            }
        }
    }
    xhr.onerror = function () {
        console.log("%c获取缓存请求出错,已停止","color:red");
        post_notification("获取缓存请求出错,已停止","red");
    }
    xhr.open("GET", "wcache.txt", true);
    xhr.send();
}

function cmd_server_cmd(cmds)
{
    cmdc=cmds.substring(cmds.indexOf(" ")+1);
    while(cmdc[0]==' ')
    {
        cmdc=cmdc.substring(1);
    }
    var sync_cmd=new XMLHttpRequest();
    sync_cmd.onload=()=>{console.log("指令成功")};
    sync_cmd.onerror=()=>{console.log("指令错误")};
    sync_cmd.open("POST","winc/cmd/run_d.slience");
    sync_cmd.send(cmdc);
    return "正在执行";
}


var retry_time=10;
var server_reconnect_time=1;

function get_state_from_server()
{
    if(server_reconnect_time)
    {
        server_reconnect_time--;
        return;
    }

    var xhr = new XMLHttpRequest();
 
    xhr.onload = function () {
        // 输出接收到的文字数据
        change_state(Number(xhr.responseText));
        set_state_box('server_state',"green","_W_服务器 : 连接正常");
        var wait_src_copy=wait_src.concat();
        wait_src=[];
        while(wait_src_copy.length)
        {
            var req_data=wait_src_copy.pop();
            req_data();
        }
    }
    
    xhr.onerror = function () {
        change_state(-1);
        set_state_box('server_state',"red","_W_服务器 : 连接错误");
        server_reconnect_time=retry_time;
        post_notification("从服务器获取状态出错","red");
    }
    
    xhr.open("GET", "/CRA/change_alerm_state.cdt", true);
    xhr.send();
    return;
}
function set_state_to_server(state_num)
{
    var xhr = new XMLHttpRequest();
 
    xhr.onload = function () {
        // 输出接收到的文字数据
        console.log("set_state_to_server:"+xhr.responseText);
    }
    
    xhr.onerror = function () {
        post_notification("向服务器设置状态出错","red");
    }
    
    xhr.open("GET", "CRA/change_alerm_state.opt?s="+state_num, true);
    xhr.send();
}

// /**
//  * @function 向服务器发送wcache.txt设置请求
//  * @param {string} set_cache 要设置的内容 
//  */
// function set_sopt_cache(set_cache)
// {
//     var xhr = new XMLHttpRequest();
//     xhr.onload = function () {
//         // 输出接收到的文字数据
//         console.log("设置缓存:"+xhr.responseText);
//     }
//     xhr.onerror = function () {
//         console.log("%c设置缓存请求出错,已停止","color:red");
//     }
//     xhr.open("POST", "wcache.txt?q=cra", true);
//     xhr.send(set_cache);
// }

var sync_handle=setInterval(get_state_from_server,1000);
link_cmds_lib();