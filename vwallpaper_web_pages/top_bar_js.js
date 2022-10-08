function rand_fan_speed()
{
    setInterval(() => {
        var fanspeed=Math.round(Math.random()*100);
        if(fanspeed<20)
        {
            document.getElementById("fan0").style.setProperty("--state_color","chartreuse");
        }
        else if(fanspeed<70)
        {
            document.getElementById("fan0").style.setProperty("--state_color","cyan");
        }
        else if(fanspeed<90)
        {
            document.getElementById("fan0").style.setProperty("--state_color","yellow");
        }
        else
        {
            document.getElementById("fan0").style.setProperty("--state_color","red");
        }
        document.getElementById("fan0").children[1].children[1].children[0].style.setProperty("--progess_value",String(100+fanspeed)+'px');
        document.getElementById("fan0").children[1].children[0].innerText="风扇0 : 开 "+String(3000+30*fanspeed)+"RPM ";
    }, 1000);
}

/**
 * @function 设置状态项状态
 * @param {string} box_id 要设置的 state_box 的 id
 * @param {string} color 要设置的颜色(green会被自动替换为亮绿色)
 * @param {string} text_content 描述内容
 */
function set_state_box(box_id,color,text_content)
{
    if(color=="green")
    {
        color="chartreuse";
    }
    document.getElementById(box_id).style.setProperty("--state_color",color);
    document.getElementById(box_id).children[1].children[0].innerText=text_content;

}

week_day_name=['日','一','二','三','四','五','六'];
week_day_name_eng=['sun','mon','tues','wedness','thurs','fri','sat'];
var week_day=-1;
var now_date_str="";
var now_time_str="";
var week_nums=0;
var now_t=new Date();
function date_update()
{
    now_t=new Date();
    var week_day_new=now_t.getDay();
    var nmouth=now_t.getMonth()+1;
    var ndate=now_t.getDate();
    now_date_str =now_t.getFullYear()+'-'+(nmouth>9?nmouth:'0'+String(nmouth))+'-'+(ndate>9?ndate:'0'+String(ndate));

    var nhours=now_t.getHours();
    var nminutes=now_t.getMinutes();
    var nsecond=now_t.getSeconds();
    now_time_str =(nhours>9?nhours:'0'+String(nhours))+':'+(nminutes>9?nminutes:'0'+String(nminutes))+':'+(nsecond>9?nsecond:'0'+String(nsecond));


    if(week_day_new!=week_day)
    {
        week_day=week_day_new;
        var xhr = new XMLHttpRequest();
        xhr.onload = function () {
            // 输出接收到的文字数据
            console.log("获取缓存:"+xhr.responseText);
            var beg_time=new Date(xhr.responseText.replaceAll("-","/"));
            week_nums = Math.floor((now_t.getTime()-beg_time.getTime())/(1000*3600*24*7))+1;
            var courses_list=document.getElementById("course_"+week_day_name_eng[week_day]+"day").getElementsByClassName("cource_grid");
            for(var num=0;num<courses_list.length;num++)
            {
                var fra=courses_list[num].getElementsByClassName("course_week_period");
                if(fra.length)
                {
                    var src_text = fra[0].innerText;
                    src_text.replace("周","");
                    var parr = src_text.split("-");
                    if(week_nums<Number(parr[0])||week_nums>Number(parr[1]))
                    {
                        courses_list[num].getElementsByClassName("course_name")[0].style.color="goldenrod";
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
        }
        xhr.open("GET", "wcache.txt", true);
        xhr.send();
    }


    document.getElementById("course_"+week_day_name_eng[week_day_new]+"day").getElementsByClassName("week_day_name")[0].style.color="yellow";
    document.getElementById("course_"+week_day_name_eng[week_day_new]+"day").getElementsByClassName("week_day_name")[0].style.backgroundColor="rgba(255,255,0,.3)";
    document.getElementById("top_bar_date").innerText=now_date_str+' '+now_time_str+' 第'+week_nums+'周 星期'+week_day_name[week_day_new];
    
}

var date_update_handle=setInterval(date_update,1000);

var notification_shining_state=0;

/**
 * @function 增加通知
 * @param {string} content 通知内容
 * @param {string} color 颜色名称(default自动替换为主题色 green替换为亮绿色)
 */
function post_notification(content,color)
{
    if(color=='default')
    {
        color="var(--top_bar_theme_color)";
    }
    if(color=="green")
    {
        color="chartreuse";
    }
    document.getElementById("notifications").innerHTML+='<div class="notification_item" style="color: '+color+';">'+now_time_str+' '+content+'</div>';
    var stop=document.getElementById("notifications").clientHeight;
    document.getElementById("notification_box").scroll({top:stop,left:0,behavior:'smooth'});


    if(notification_shining_state)
    {
        clearTimeout(notification_shining_state);
    }
    document.getElementById("notification_contorler").checked=1;
    setTimeout(() => {
        document.getElementById("notification_contorler").checked=0;
        setTimeout(() => {
            var stop=document.getElementById("notifications").clientHeight;
            document.getElementById("notification_box").scroll({top:stop,left:0,behavior:'smooth'});
        }, 1000);
    }, 3000);
}