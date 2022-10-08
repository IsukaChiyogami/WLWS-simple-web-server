//TODO: 任务单位增加进度条背景 task的finish状态
var task_data=[];
var task_data_time_set=[];
var task_painting=false;

/**
 * 
 * @param {Object} sele 操作元素
 * @param {Number} tnum 任务id
 * @returns 无
 */
function task_finish(sele,tnum)
{
    if(tnum==-1)
    {
        return;
    }
    if(task_data[tnum].finish)
    {
        sele.style.color="red";
        sele.style.borderColor="red";
        sele.innerText="任务未完成(单击设置为完成)";
        task_data[tnum].finish=false;
    }
    else
    {
        sele.style.color="greenyellow";
        sele.style.borderColor="greenyellow";
        sele.innerText="任务已完成(单击设置为未完成)";
        task_data[tnum].finish=true;
    }
}
//检查任务状态，处理从[未开始]到[开始]这样的状态转变
function refresh_tasks()
{
    if(task_painting)
    {
        return;
    }
    var warning_num=0;
    var whether_report=false;
    for(num=0;num<task_data_time_set.length;num++)
    {
        var state="",right_notice="";
        var nott=task_data_time_set[num].nott;
        var begt=task_data_time_set[num].begt;
        var endt=task_data_time_set[num].endt;
        //classify
        if(task_data[num].finish)
        {
            if(now_t<endt)
            {
                state="finish";
                if(now_t<begt)
                {
                    right_notice="距离开始:"+diff_to_duration_str(begt.getTime()-now_t.getTime());
                }
                else
                {
                    right_notice="距离结束:"+diff_to_duration_str(endt.getTime()-now_t.getTime());
                }
            }
            else
            {
                state="past";
                right_notice="已经结束:"+diff_to_duration_str(now_t.getTime()-endt.getTime());
            }
        }
        else
        {
            if(now_t<nott)
            {
                state="future";
                right_notice="距离开始:"+diff_to_duration_str(begt.getTime()-now_t.getTime());
            }
            else
            {
                if(now_t<begt)
                {
                    state="upcoming";
                    right_notice="距离开始:"+diff_to_duration_str(begt.getTime()-now_t.getTime());
                }
                else
                {
                    if(now_t<endt)
                    {
                        state="running";
                        right_notice="距离结束:"+diff_to_duration_str(endt.getTime()-now_t.getTime());
                    }
                    else
                    {
                        state="warning";
                        right_notice="已经结束:"+diff_to_duration_str(now_t.getTime()-endt.getTime());
                        warning_num++;
                    }
                }
            }
        }
        if(state!=task_data_time_set[num].state)
        {
            whether_report=true;
            //移动并更改间隔提示
            var new_box=document.createElement("div");
            new_box.className="task_item";
            new_box.id="task_item_"+num;
            new_box.innerHTML=document.getElementById("task_item_"+num).innerHTML;
            document.getElementById("task_item_"+num).remove();
            document.getElementById(state+"_tasks").appendChild(new_box);
            task_data_time_set[num].state=state;
        }
        document.getElementById("task_item_"+num).getElementsByClassName("task_distance")[0].innerText=right_notice;
    }
    if(whether_report&&warning_num)
    {
        post_notification("你有"+warning_num+"项任务逾期!","orangered");
    }
}


var tasks_check_handle=0;

/**
 * 重新解析并重绘任务栏
 * @param {string} task_json_str 数据json字符串
 */
function parse_tasks(task_json_str)
{
    task_painting=true;
    
    document.getElementById("future_tasks").innerHTML='<div class="task_class_title_bg"><div class="task_class_title">计划</div></div>';
    document.getElementById("upcoming_tasks").innerHTML='<div class="task_class_title_bg"><div class="task_class_title">提醒</div></div>';
    document.getElementById("running_tasks").innerHTML='<div class="task_class_title_bg"><div class="task_class_title">进行中</div></div>';
    document.getElementById("finish_tasks").innerHTML='<div class="task_class_title_bg"><div class="task_class_title">完成</div></div>';
    document.getElementById("warning_tasks").innerHTML='<div class="task_class_title_bg"><div class="task_class_title">警告</div></div>';
    document.getElementById("past_tasks").innerHTML='<div class="task_class_title_bg"><div class="task_class_title">过去</div></div>';

    task_data=JSON.parse(task_json_str).tasks;
    task_data_time_set=[];
    var warning_num=0;
    for(var num=0;num<task_data.length;num++)
    {
        var time_set_cr={state:"",nott:new Date(task_data[num].nott),begt:new Date(task_data[num].begt),endt:new Date(task_data[num].endt)};
        var nott=time_set_cr.nott;
        var begt=time_set_cr.begt;
        var endt=time_set_cr.endt;
        if(task_data[num].finish)
        {
            if(now_t<endt)
            {
                time_set_cr.state="finish";
            }
            else
            {
                time_set_cr.state="past";
            }
        }
        else
        {
            if(now_t<nott)
            {
                time_set_cr.state="future";
            }
            else
            {
                if(now_t<begt)
                {
                    time_set_cr.state="upcoming";
                }
                else
                {
                    if(now_t<endt)
                    {
                        time_set_cr.state="running";
                    }
                    else
                    {
                        time_set_cr.state="warning";
                        warning_num++;
                    }
                }
            }
        }
        task_data_time_set.push(time_set_cr);
        document.getElementById(time_set_cr.state+"_tasks").innerHTML+=task_to_html(task_data[num],num,"task_item_"+num,time_set_cr.nott,time_set_cr.begt,time_set_cr.endt);
    }
    if(warning_num)
    {
        post_notification("你有"+warning_num+"项任务逾期!","orangered");
    }
    task_painting=false;
}

function get_tasks_form_server()
{
    var tdr=new XMLHttpRequest();
    tdr.onload=()=>{parse_tasks(tdr.responseText)};
    tdr.onerror=()=>{append_src_req_func(get_tasks_form_server)};
    tdr.open("GET","tasks_data.json");
    tdr.send();
}


/**
 * 
 * @param {Object} target_ele 检查的元素
 * @param {string} default_content 默认内容
 * @returns 无内容返回false,有则true
 */
function check_empty(target_ele,default_content)
{
    if(target_ele.innerText=="")
    {
        target_ele.innerText=default_content;
        return false;
    }
    return true;
}

/**
 * 
 * @param {Object} cele 所解析元素字符串
 * @param {string} title 报错时标题
 * @param {string} source_ele 起始时间所在元素
 * @param {string} target_ele 持续时间所在元素
 * @returns 无返回值
 */
function parse_time_str(cele,title,source_ele,target_ele)
{
    var test_d=new Date(cele.innerText);
    if(test_d=='Invalid Date')
    {
        document.getElementById("tcnotice_c").innerText=title+"格式错误";
        return;
    }
    if(source_ele && target_ele)
    {
        var beg_t=new Date(document.getElementById(source_ele).innerText);
        if(beg_t=='Invalid Date')
        {
            document.getElementById("tcnotice_c").innerText=title+"格式错误";
            return;
        }
        var tds=(test_d.getTime()-beg_t.getTime());
        if(tds<0)
        {
            document.getElementById("tcnotice_c").innerText=title+"时间错误";
            return;
        }
        
        document.getElementById(target_ele).innerText=diff_to_duration_str(tds);
    }
    document.getElementById("tcnotice_c").innerText="没有发生错误";
}

function diff_to_duration_str(tdiff)
{
    tds=Math.floor(tdiff/1000)
    var tdss=String(tds%60)+"秒";
    var tds=Math.floor(tds/60);
    if(tds)
    {
        tdss=String(tds%60)+"分钟"+tdss;
        tds=Math.floor(tds/60);
        if(tds)
        {
            tdss=String(tds%24)+"小时"+tdss;
            tds=Math.floor(tds/24);
            if(tds)
            {
                tdss=String(tds)+"天"+tdss;
            }
        }
    }
    return tdss;
}

/**
 * 
 * @param {Object} cele 所解析元素字符串
 * @param {string} title 报错时标题
 * @param {string} source_ele 起始时间所在元素
 * @param {string} target_ele 持续时间所在元素
 * @param {boolean} backwrad 是否反向计算
 * @returns 无返回值
 */
function parse_duration_str(cele,title,source_ele,target_ele,backwrad)
{
    var src=cele.innerText.replace("日","天").replace("小时","时").replace("分钟","分").replace("秒钟","秒");
    var ipos=src.indexOf("天");
    var qdii=0,check_buf=0;
    
    var test_d=new Date(document.getElementById(source_ele).innerText);
    if(test_d=='Invalid Date')
    {
        document.getElementById("tcnotice_c").innerText=title+"起始时间格式错误";
        return;
    }
    if(ipos>-1)
    {
        check_buf=Number(src.substr(0,ipos));
        if(isNaN(check_buf))
        {
            document.getElementById("tcnotice_c").innerText=title+"格式错误";
            return;
        }
        else
        {
            qdii=check_buf*24*3600;
        }
        src=src.substr(ipos+1);
    }

    var ipos=src.indexOf("时");
    if(ipos>-1)
    {
        check_buf=Number(src.substr(0,ipos));
        if(isNaN(check_buf))
        {
            document.getElementById("tcnotice_c").innerText=title+"格式错误";
            return;
        }
        else
        {
            qdii+=check_buf*3600
        }
        src=src.substr(ipos+1);
    }

    var ipos=src.indexOf("分");
    if(ipos>-1)
    {
        check_buf=Number(src.substr(0,ipos));
        if(isNaN(check_buf))
        {
            document.getElementById("tcnotice_c").innerText=title+"格式错误";
            return;
        }
        else
        {
            qdii+=check_buf*60;
        }
        src=src.substr(ipos+1);
    }
    var ipos=src.indexOf("秒");
    if(ipos>-1)
    {
        check_buf=Number(src.substr(0,ipos));
        if(isNaN(check_buf))
        {
            document.getElementById("tcnotice_c").innerText=title+"格式错误";
            return;
        }
        else
        {
            qdii+=check_buf;
        }
    }
    if(backwrad)
    {
        qdii=-qdii;
    }
    test_d.setTime(test_d.getTime()+qdii*1000);
    qdii=test_d.getMonth()+1;
    var rets=test_d.getFullYear()+'-'+(qdii>9?qdii:('0'+String(qdii)))+'-';
    qdii=test_d.getDate();
    rets+=(qdii>9?qdii:('0'+String(qdii)))+' ';
    qdii=test_d.getHours();
    rets+=(qdii>9?qdii:('0'+String(qdii)))+':';
    qdii=test_d.getMinutes();
    rets+=(qdii>9?qdii:('0'+String(qdii)))+":";
    qdii=test_d.getSeconds();
    rets+=(qdii>9?qdii:('0'+String(qdii)));
    document.getElementById(target_ele).innerText=rets;
    document.getElementById("tcnotice_c").innerText="没有发生错误";
}
var creating_task=new Object();
var task_preview_refresh_handle=0;
function create_task_func()
{
    var begt=new Date(document.getElementById('tcbegt_c').innerText);
    var endt=new Date(document.getElementById('tcendt_c').innerText);
    var nott=new Date(document.getElementById('tcnotts_c').innerText);
    if(begt!='Invalid Date'&&endt!='Invalid Date'&&nott!='Invalid Date')
    {
        if(nott.getTime()<=begt.getTime() && begt.getTime()<=endt.getTime())
        {
            creating_task={
                title:document.getElementById("tcname_c").innerText,
                content:document.getElementById("tccontent_c").innerText,
                begt:document.getElementById("tcbegt_c").innerText,
                endt:document.getElementById("tcendt_c").innerText,
                nott:document.getElementById("tcnotts_c").innerText
            }
            var task_preview_refresh_handle=setInterval(() => {
                var preview_rn='';
                if(now_t<nott)
                {
                    preview_rn="距离开始:"+diff_to_duration_str(begt.getTime()-now_t.getTime());
                }
                else
                {
                    if(now_t<begt)
                    {
                        preview_rn="距离开始:"+diff_to_duration_str(begt.getTime()-now_t.getTime());
                    }
                    else
                    {
                        if(now_t<endt)
                        {
                            preview_rn="距离结束:"+diff_to_duration_str(endt.getTime()-now_t.getTime());
                        }
                        else
                        {
                            preview_rn="已经结束:"+diff_to_duration_str(now_t.getTime()-endt.getTime());
                        }
                    }
                }
                document.getElementById("creating_task_preview").getElementsByClassName("task_distance")[0].innerText=preview_rn;
            }, 1000);
            important_noticing(0,0,'<div id="task_preview_title">预览</div>'+task_to_html(creating_task,-1,"creating_task_preview",nott,begt,endt)+'<div id="sec_cfm" onclick="upload_task()">确认</div>',()=>{clearInterval(task_preview_refresh_handle);});
            // TODO : 展示二次确认，展示进程，上传服务器，展示结果
        }
        else
        {
            document.getElementById("tcnotice_c").innerText="提醒时间不得超过开始时间,结束时间不得不超过开始时间";
        }
    }
    else
    {
        document.getElementById("tcnotice_c").innerText="时间格式错误";
    }
}

function upload_task()
{
    var nele=document.getElementById("sec_cfm");
    nele.style.userSelect='none';
    nele.innerText="正在与服务器同步";
    var send_tl=new Object();
    send_tl.tasks=task_data.concat(creating_task);
    var stlr=new XMLHttpRequest();
    stlr.onload=()=>{
        var nele=document.getElementById("sec_cfm");
        nele.style.userSelect='auto';
        nele.innerText="成功";
        parse_tasks(stlr.responseText);
        setTimeout(run_close_func, 100);
    };
    stlr.onerror=()=>{
        var nele=document.getElementById("sec_cfm");
        nele.style.userSelect='auto';
        nele.innerText="失败,请稍后再试";
    };
    stlr.open("POST","winc/upload/tasks_data.json");
    stlr.send(JSON.stringify(send_tl));
}

/**
 * 
 * @param {Object} task_data_uint 任务数据单元
 * @param {Number} id_num id数字
 * @param {string} id_str id字符串
 * @param {Date} nottt 解析完成的提醒时间
 * @param {Date} begtt 解析完成的开始时间
 * @param {Date} endtt 解析完成的结束时间
 * @returns 返回html str
 */
function task_to_html(task_data_uint,id_num,id_str,nottt,begtt,endtt)
{
    var right_notice="";
    if(now_t<begtt)
    {
        right_notice="距离开始:"+diff_to_duration_str(begtt.getTime()-now_t.getTime());
    }
    else
    {
        if(now_t<endtt)
        {
            right_notice="距离结束:"+diff_to_duration_str(endtt.getTime()-now_t.getTime());
        }
        else
        {
            right_notice="已经结束:"+diff_to_duration_str(now_t.getTime()-endtt.getTime());
        }
    }
    colstr="";
    contstr="";
    if(task_data_uint.finish)
    {
        colstr="greenyellow";
        contstr="任务已完成(单击设置为未完成)";
    }
    else
    {
        colstr="red";
        contstr="任务未完成(单击设置为已完成)";
    }
    return '<div class="task_item" id="'+id_str+
        '"><div class="task_finish_state_bar" style="color: '+colstr+
        ';border-color:'+colstr+';" onclick="task_finish(this,'+id_num+
        ')">'+contstr+'</div><div style="display: block;"><div class="task_title">'+
        task_data_uint.title+'</div><div class="task_distance">'+right_notice+
        '</div></div><div style="display: block;"><div class="task_nottd">开始时间'+
        task_data_uint.begt+'</div>'+'<div class="task_duration">持续:'+
        diff_to_duration_str(endtt.getTime()-begtt.getTime())+
        '</div><div class="task_endtime">结束时间:'+task_data_uint.endt+
        '</div></div><div style="display: block;"><div class="task_nottd">开始'+
        diff_to_duration_str(begtt.getTime()-nottt.getTime())+
        '前提醒</div><div class="task_notts">提醒时间:'+task_data_uint.nott+
        '</div></div><div class="task_content">'+task_data_uint.content+'</div></div>';
}

get_tasks_form_server();
var right_bar_thread_handle=setInterval(() => {
    if(task_data.length)
    {
        refresh_tasks();
    }
}, 1000);