var wait_src=[]//get state form server 成功时自动请求资源 每个单元包括 .url路径与.hfunc结果处理函数 .errn错误报告内容 .keepR false-只请求一次 true-错误时保持继续请求

function append_src_req(hfunc)
{
    wait_src.push(hfunc);
}

function change_state(num_level)
{
    switch(num_level)
    {
        case -1:
            document.getElementById("d0").style.setProperty("--global_bg_color","rgba(127, 127, 127, 0.3)");
            document.getElementById("d0").style.setProperty("--global_theme_color","gray");
            document.getElementById("main_state_name").innerText="无法确认状态";
            break;
        case 0:
            document.getElementById("d0").style.setProperty("--global_bg_color","rgba(0, 255, 255, 0.3)");
            document.getElementById("d0").style.setProperty("--global_theme_color","cyan");
            document.getElementById("main_state_name").innerText="日常办公状态";
            break;
        case 1:
            document.getElementById("d0").style.setProperty("--global_bg_color","rgba(255, 0, 0, 0.3)");
            document.getElementById("d0").style.setProperty("--global_theme_color","red");
            document.getElementById("main_state_name").innerText="一级戒备状态";
            break;
        case 2:
            document.getElementById("d0").style.setProperty("--global_bg_color","rgba(255, 69, 0, 0.3)");
            document.getElementById("d0").style.setProperty("--global_theme_color","orangered");
            document.getElementById("main_state_name").innerText="二级戒备状态";
            break;
        case 3:
            document.getElementById("d0").style.setProperty("--global_bg_color","rgba(255, 165, 0, 0.3)");
            document.getElementById("d0").style.setProperty("--global_theme_color","orange");
            document.getElementById("main_state_name").innerText="三级戒备状态";
            break;
        case 4:
            document.getElementById("d0").style.setProperty("--global_bg_color","rgba(255, 192, 203, 0.3)");
            document.getElementById("d0").style.setProperty("--global_theme_color","pink");
            document.getElementById("main_state_name").innerText="休闲娱乐状态";
            break;
        default:
            break;
    }
}
function break_bubble(event)
{
    event.stopPropagation();
}
var clsoe_func_handle=()=>{};
function run_close_func()
{
    clsoe_func_handle();
    document.getElementById("importsnt_notice_bg").style.display="none";
}
/**
 * 
 * @param {string} content 内容,Excontent非零且此项置零则不执行post_notification
 * @param {string} top_bar_notice_color 用于post_notification的color
 * @param {string} Excontent 如果非0,则用于important_notice的内容(innerHTML)为此,否则以content做important_notice的innerText
 * @param {function} clsoe_func 被关闭时执行的函数
 * @return 返回作为容纳框的元素
 */
function important_noticing(content,top_bar_notice_color,Excontent,clsoe_func)
{
    document.getElementById("importsnt_notice_bg").style.display='block';
    if(Excontent)
    {
        document.getElementById("important_notice").innerHTML=Excontent;
        if(content)
        {
            post_notification(contnet,top_bar_notice_color);
        }
    }
    else
    {
        document.getElementById("important_notice").innerText=content;
        post_notification(contnet,top_bar_notice_color);
    }
    if(clsoe_func)
    {
        clsoe_func_handle=clsoe_func;
    }
    return document.getElementById("important_notice");
}
console.log("%c当你看到这段文字,你就会看到这段文字\n%cQQ:3518767065", "color:orange","color:cyan");