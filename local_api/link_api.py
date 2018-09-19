#!flask/bin/python
from flask import Flask, jsonify, request
import requests
import json


app = Flask(__name__)
@app.route('/link', methods=['POST'])


def link():
    input_data = request.get_json(force=True)
    link = input_data['link']
    link = link.split("/")[-1]
    age = "9-11"
    price = "50-75"
    interest = "cat1500028"

    url_start = "https://sp1004f888.guided.ss-omtrdc.net/?do=json-db&callback=json&count=18&cc=us&i=1&jsonp=jsonCallback&lang=en&pt=shop&q=*&q1="
    url_end = "&rank=rank_vrs&sp_q_exact_9=us&userquery=*&x1=price-range&x2=age-range&x3=interests_id&jsonp=jsonCallback"
    url = url_start + price + "&q2=" + age + "&q3=" + interest + url_end

    ref_url_start = "https://shop.lego.com/en-US/search/*?callback=json&cc=us&count=18&do=json-db&i=1&jsonp=jsonCallback&lang=en&pt=shop&q=*&q1="
    ref_url_end = "&rank=rank_vrs&sp_q_exact_9=us&userquery=*&x1=price-range&x2=age-range&x3=interests_id"
    ref_url = ref_url_start + price + "&q2=" + age + "&q3=" + interest + ref_url_end

    headers = {
        'Accept-Encoding': 'gzip, deflate, br',
        'Accept-Language': 'en-US,en;q=0.9',
        'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.89 Safari/537.36',
        'Accept': '*/*',
        'Referer': ref_url,
        'Connection': 'keep-alive'
    }

    def get():
        response = requests.get(url=url, headers=headers)
        resp = response.content
        resp = resp.replace("jsonCallback( ", "")[:-2]
        data = json.loads(resp)
        return data['results']

    results = get()

    match = 0
        
    for i in xrange(len(results)):
        if link == results[i]['seo_path']:
            match = 1
    
    return jsonify({'match':match, 'link':link})


if __name__ == '__main__':
    app.run(debug=True)

