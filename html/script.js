var resOb;

function genXMLHttpRequestObject()
{
	try 
	{
		resOb = new XMLHttpRequest();
	}
	catch( Error )
	{
		alert("Cannot create XMLHttpRequest Object, wrong browsers?");
	}
	
	return resOb;
}

function sendReq()
{
	resOb.open( 'get', 'ajax1.txt', true )
	resOb.onreadystatechange = handleResponse;
	resOb.send( null );
}

function handleResponse()
{
	if(resOb.readyState == 4)
	{
		document.getElementById( "ajax_response").innerHTML = 
			resOb.responseText;
	}
}

function hello()	
{
	document.write("<h1>AJAX Demo</h1>");
}

resOb = genXMLHttpRequestObject();